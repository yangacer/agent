#include "agent/agency_base.hpp"
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "connection.hpp"
#include "session.hpp"
#include "agent/parser.hpp"
#include "agent/log.hpp"
#include "agent/pre_compile_options.hpp"

#define PICK_SERVICE (*io_service_pool_.get_io_service())

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#else
#   define AGENT_TRACKING(Desc)
#endif

namespace asio = boost::asio;

agency_base::agency_base(std::string const &address, std::string const &port, 
                         std::size_t thread_number)
: io_service_pool_(thread_number),
  signal_set_( PICK_SERVICE ),
  acceptor_( PICK_SERVICE ),
  handlers_()
{
  AGENT_TRACKING("agency_base::ctor");
  // setup signal handling
  signal_set_.add(SIGINT);
  signal_set_.add(SIGTERM);
  signal_set_.async_wait(boost::bind(&agency_base::handle_stop, this));
  // prepare accept
  tcp::resolver resolver(acceptor_.get_io_service());
  tcp::resolver::query query(address, port);
  tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  start_accept();
}

agency_base::~agency_base()
{
  AGENT_TRACKING("agency_base::dtor");
  handlers_.clear();
}

void agency_base::set_handler(std::string const & uri_prefix, 
                              std::string const &method, handler_type handler)
{
  try {
    handlers_.at(method).at(uri_prefix) = handler;
  } catch ( std::out_of_range &) {}

  return;
}

void agency_base::add_handler(std::string const & uri_prefix, 
                              std::string const &method, handler_type handler)
{
  handlers_[method][uri_prefix] = handler;
}

void agency_base::del_handler(std::string const & uri_prefix,
                              std::string const &method)
{
  handlers_[method].erase(uri_prefix);
}


agency_handler_type agency_base::get_handler(std::string const &uri, 
                                             std::string const &method)
{
  auto i = handlers_[method].lower_bound(uri);
  if(i != handlers_[method].end() &&
     0 == uri.find(i->first)) // matched
    return i->second;
  else
    return handlers_["GET"]["_forbidden_"];
}

void agency_base::run()
{
  io_service_pool_.run(io_service_pool::WITHOUT_WORK); 
}


void agency_base::async_reply(http::request const &request, 
                              http::response const &response, 
                              session_ptr session,
                              handler_type handler)
{
  AGENT_TRACKING("agency_base::async_reply");
  auto req_connection_policy = http::find_header(request.headers, "Connection");
  
  std::ostream os(&session->io_buffer);
  http::response cp = response;

  if( req_connection_policy != request.headers.end() ) {
    cp.headers << *req_connection_policy;
  } else {
    cp.headers << http::entity::field("Connection", "close");
  }
  os << cp;
#ifdef AGENT_AGENCY_LOG_HEADERS
  logger::instance().async_log(
    "response headers", true,
    session->connection->socket().remote_endpoint(),
    cp);
#endif
  session->io_handler = boost::bind(
    &agency_base::handle_reply, this, _1, request, session, handler);
  session->connection->write(*session);
}

void agency_base::handle_reply(boost::system::error_code const &err,
                               http::request const &request,
                               session_ptr session,
                               handler_type handler)
{
  AGENT_TRACKING("agency_base::handle_reply");
  asio::io_service &ios = session->connection->get_io_service();
  if(err) session->connection.reset();
  notify(err, request, ios, session, asio::const_buffer(0,0), handler);
}

void agency_base::async_reply_chunk(http::request const &request,
                                    session_ptr session, 
                                    boost::asio::const_buffer buffer, 
                                    handler_type handler)
{
  AGENT_TRACKING("agency_base::async_reply_chunk");
  session->io_handler = boost::bind(
    &agency_base::handle_reply_chunk, this, _1, request, session, handler);
  session->connection->write(*session, buffer);
}

void agency_base::handle_reply_chunk(boost::system::error_code const &err,
                                     http::request const &request,
                                     session_ptr session,
                                     handler_type handler)
{
  AGENT_TRACKING("agency_base::handle_reply_chunk");
  asio::io_service &ios = session->connection->get_io_service();
  if(err) session->connection.reset();
  notify(err, request, ios, session, handler);
}

void agency_base::async_reply_commit(http::request const &request, 
                                     session_ptr session,
                                     handler_type handler)
{
  AGENT_TRACKING("agency_base::async_reply_commit");
  handle_reply_commit(boost::system::error_code(), request, session, handler);
}

void agency_base::async_reply_commit(http::request const &request,
                                     http::response const &response, 
                                     session_ptr session,
                                     handler_type handler)
{
  AGENT_TRACKING("agency_base::async_reply_commit (atomic)");
  auto req_connection_policy = http::find_header(request.headers, "Connection");
  
  std::ostream os(&session->io_buffer);
  http::response cp = response;

  if( req_connection_policy != request.headers.end() ) {
    cp.headers << *req_connection_policy;
  } else {
    cp.headers << http::entity::field("Connection", "close");
  }
  os << cp;
#ifdef AGENT_AGENCY_LOG_HEADERS
  logger::instance().async_log(
    "response headers", true,
    session->connection->socket().remote_endpoint(),
    cp);
#endif
  session->io_handler = boost::bind(
    &agency_base::handle_reply_commit, this, _1, request, session, handler);
  session->connection->write(*session);
}


void agency_base::handle_reply_commit(boost::system::error_code const &err, 
                                      http::request const &request,
                                      session_ptr session,
                                      handler_type handler)
{
  AGENT_TRACKING("agency_base::handle_reply_commit");
  asio::io_service &ios = session->connection->get_io_service();
  if(!err) {
    auto connection_policy = http::find_header(request.headers, "Connection");
    if( connection_policy == request.headers.end() || 
        connection_policy->value == "close" || 
        connection_policy->value == "Close")
    {
      session->connection.reset();
      notify(err, request, ios, session, asio::const_buffer(0,0), handler);
    } else { 
      // keep-alive
      notify(err, request, ios, session, asio::const_buffer(0,0), handler);
      session->quality_config.read_kb(10);
      start_read(session);
    }
  } else {
    session->connection.reset();
    notify(err, http::request(), ios, session, asio::const_buffer(0,0), handler);
  }
}

void agency_base::start_accept()
{
  AGENT_TRACKING("agency_base::start_accept");
  connection_.reset(new connection(PICK_SERVICE));
  acceptor_.async_accept(connection_->socket(),
                         boost::bind(&agency_base::handle_accept, this, _1));
}

void agency_base::handle_accept(boost::system::error_code const &err)
{
  AGENT_TRACKING("agency_base::handle_accept");
  if(!err) {
    boost::system::error_code ec;
    connection_->socket().set_option(
      connection::socket_type::keep_alive(true), ec);
    // create session
    if(!ec) {
      session_ptr session(new session_type(connection_->get_io_service()));
      session->quality_config.read_kb(1);
      session->connection = connection_;
      start_read(session);
    }
  }
  start_accept();
}

void agency_base::start_read(session_ptr session)
{
  AGENT_TRACKING("agency_base::start_read");
  // read request (headers only)
  session->io_handler = boost::bind(
    &agency_base::handle_read_request_line, this, _1, _2, session);
  session->connection->read_until("\r\n", 4096, *session);
}

void agency_base::handle_read_request_line(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  session_ptr session)
{
  AGENT_TRACKING("agency_base::handle_read_request_line");
  if(!err) {
    auto beg(asio::buffers_begin(session->io_buffer.data())),
         end(asio::buffers_end(session->io_buffer.data())),
         orig = beg;
    http::request request;
    if(!http::parser::parse_request_first_line(beg, end, request))
      return;
    session->io_buffer.consume(beg - orig);
    session->io_handler = boost::bind(
      &agency_base::handle_read_header_list, this, 
      _1, _2, session, request);
    session->connection->read_until("\r\n\r\n", 4096, *session);   
  }
}

void agency_base::handle_read_header_list(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  session_ptr session,
  http::request request)
{
  AGENT_TRACKING("agency_base::handle_read_header_list");
  if(!err) {
    auto beg(asio::buffers_begin(session->io_buffer.data())),
         end(asio::buffers_end(session->io_buffer.data())),
         orig = beg;
    if(!http::parser::parse_header_list(beg, end, request.headers)) 
      return;
    session->io_buffer.consume(beg - orig);
#ifdef AGENT_AGENCY_LOG_HEADERS
  logger::instance().async_log(
    "request headers", true, 
    session->connection->socket().remote_endpoint(), request);
#endif
    boost::system::error_code agency_error = err;
    auto content_length = http::find_header(request.headers, "Content-Length");
    
    if( content_length != request.headers.end() ) 
      session->expected_size =   
        content_length->value_as<boost::intmax_t>();
    else
      session->expected_size = 0;

    if(session->expected_size) {
      session->io_handler = boost::bind(
        &agency_base::handle_read_body, this, _1, _2, session, request);
      session->connection->read_some(1, *session);
    } else {
      agency_error = boost::asio::error::eof;
    }
    
    notify(agency_error, request, session->connection->get_io_service(), 
           session, asio::const_buffer(0,0), get_handler(
             request.query.path, request.method));
  }
}

void agency_base::handle_read_body(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  session_ptr session,
  http::request const &request)
{
  AGENT_TRACKING("agency_base::handle_read_body");
  // XXX Do I need to support chunked encoding?
  session->expected_size -= length;
  asio::io_service &ios = session->connection->get_io_service();
  
  if(session->expected_size && !err) {
    session->io_handler = boost::bind(
      &agency_base::handle_read_body, this, _1, _2, session, request);
    session->connection->read_some(1, *session);
  } else { 
    session->connection.reset();
    asio::const_buffer buf(
      asio::buffer_cast<char const*>(session->io_buffer.data()), 
      session->io_buffer.size());
    notify(err, request, ios, session, buf,
           get_handler(request.query.path, request.method));
  }
}

void agency_base::handle_stop()
{
  AGENT_TRACKING("agency_base::handle_stop");
  io_service_pool_.stop(); 
}
