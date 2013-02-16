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
                              session_token_type session_token,
                              handler_type handler)
{
  
}

void agency_base::async_reply_chunk(session_token_type session_token, 
                                    boost::asio::const_buffer buffer, 
                                    handler_type handler)
{
  
}

void agency_base::async_reply_commit(http::request const &request, 
                                     session_token_type session_token, 
                                     handler_type handler)
{
  
}

void agency_base::async_reply_commit(http::request const &request,
                                     http::response const &response, 
                                     session_token_type session_token,
                                     handler_type handler)
{
  AGENT_TRACKING("agency_base::async_reply_commit");
  connection_ptr conn = boost::static_pointer_cast<connection>(session_token);
  auto i = sessions_.find(conn);
  if(i == sessions_.end()) return;
  auto &session = i->second;
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
    conn->socket().remote_endpoint(),
    response);
#endif
  session->io_handler = boost::bind(
    &agency_base::handle_reply_commit, this, _1, request, session_token, handler);
  conn->write(*session);
}


void agency_base::handle_reply_commit(boost::system::error_code const &err, 
                                      http::request const &request,
                                      session_token_type session_token,
                                      handler_type handler)
{
  AGENT_TRACKING("agency_base::handle_reply_commit");
  connection_ptr conn = boost::static_pointer_cast<connection>(session_token);
  auto i = sessions_.find(conn);
  if(i == sessions_.end()) return;
  auto &session = i->second;
  auto connection_policy = http::find_header(request.headers, "Connection");
  if( connection_policy == request.headers.end() || 
      connection_policy->value == "close" || 
      connection_policy->value == "Close")
  {
    asio::io_service &ios = conn->get_io_service();
    sessions_.erase(conn);
    conn.reset();
    notify(err, request, ios, conn, handler);
  } else { 
    // keep-alive
    notify(err, request, conn->get_io_service(), conn, handler);
    session->quality_config.read_kb(10);
    start_read(session);
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
      auto &session = sessions_[connection_];
      session.reset(new session_type(PICK_SERVICE));
      session->quality_config.read_kb(1);
      start_read(session);
    }
  }
  start_accept();
}

void agency_base::start_read(session_ptr session)
{
  // read request (headers only)
  session->io_handler = boost::bind(
    &agency_base::handle_read_request_line, this, _1, _2, connection_);
  connection_->read_until("\r\n", 4096, *session);
}

void agency_base::handle_read_request_line(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  connection_ptr connection)
{
  AGENT_TRACKING("agency_base::handle_read_request_line");
  if(!err) {
    auto &session = sessions_.at(connection);
    auto beg(asio::buffers_begin(session->io_buffer.data())),
         end(asio::buffers_end(session->io_buffer.data())),
         orig = beg;
    http::request request;
    if(!http::parser::parse_request_first_line(beg, end, request)) {
      sessions_.erase(connection);
      return;
    } 
    session->io_buffer.consume(beg - orig);
    session->io_handler = boost::bind(
      &agency_base::handle_read_header_list, this, _1, _2, connection, request);
    connection->read_until("\r\n\r\n", 4096, *session);   
  } else {
    sessions_.erase(connection);
  }
}

void agency_base::handle_read_header_list(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  connection_ptr connection,
  http::request request)
{
  AGENT_TRACKING("agency_base::handle_read_header_list");
  if(!err) {
    auto &session = sessions_.at(connection);
    auto beg(asio::buffers_begin(session->io_buffer.data())),
         end(asio::buffers_end(session->io_buffer.data())),
         orig = beg;
    if(!http::parser::parse_header_list(beg, end, request.headers)) {
      sessions_.erase(connection);
      return;
    }
    session->io_buffer.consume(beg - orig);
#ifdef AGENT_AGENCY_LOG_HEADERS
  logger::instance().async_log(
    "request headers", true, 
    connection->socket().remote_endpoint(), request);
#endif
    notify(err, request, connection->get_io_service(), connection, 
           get_handler(request.query.path, request.method));
  } else {
    sessions_.erase(connection);
  }
}

void agency_base::handle_stop()
{
  io_service_pool_.stop(); 
}
