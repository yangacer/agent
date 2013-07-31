#include "agent/agent_base_v2.hpp"
#include <string>
#include <sstream>
#include <cstring>
#include <ctime>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "agent/parser.hpp"
#include "agent/generator.hpp"
#include "agent/pre_compile_options.hpp"
#include "agent/log.hpp"
#include "agent/version.hpp"
#include "connection.hpp"
#include "session.hpp"
#include "multipart.hpp"

#define USER_AGENT_STR "OokonHTTPAgent Version"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#   define AGENT_TRACKING_2(Desc, Extra) \
    logger::instance().async_log(Desc, false, (void*)this, Extra);
#else
#   define AGENT_TRACKING(X)
#   define AGENT_TRACKING_2(X)
#endif

std::string 
determine_service_v2(http::entity::url const & url)
{
  return (url.port) ?
    boost::lexical_cast<std::string>(url.port) :
    url.scheme
    ;
}

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

agent_base_v2::context::context(
  boost::asio::io_service &ios,
  http::request const &request_,
  bool chunked_callback,
  handler_type const &handler,
  monitor_type const &monitor)
  : request(request_),
  redirect_count(0), 
  chunked_callback(chunked_callback),
  expected_size(0),
  is_redirecting(false),
  session(new session_type(ios)),
  handler(handler),
  monitor(monitor),
  qos(),
  receive_since_last_limit_check(0)
{
  using http::get_header;
  if(request.method == "POST") {
    mpart.reset(new multipart(request.query.query_map));
    request.query.query_map.clear();
    auto content_type = get_header(request.headers, "Content-Type");
    auto content_length = get_header(request.headers, "Content-Length");
    content_type->value = "multipart/form-data; boundary=" + mpart->boundary();
    content_length->value_as(mpart->size());
  }
}

agent_base_v2::context::~context()
{
  AGENT_TRACKING("agent_base_v2::context dtor");
}

agent_base_v2::agent_base_v2(asio::io_service &io_service)
: io_service_(io_service),
  resolver_(io_service)
{
  AGENT_TRACKING("agent_base_v2::ctor");
}

agent_base_v2::~agent_base_v2()
{
  AGENT_TRACKING("agent_base_v2::dtor");
}

void agent_base_v2::async_request(
  http::entity::url const &url,
  http::request request, 
  std::string const &method,
  bool chunked_callback,
  handler_type handler,
  monitor_type monitor)
{
  request.query = url.query;
  request.method = method;
  auto host = http::get_header(request.headers, "Host");
  host->value = url.host;
  if(url.port)
    host->value += ":" + boost::lexical_cast<std::string>(url.port);
  
  context_ptr ctx(new context(
      io_service_, request, chunked_callback, handler, monitor));
  
  tcp::resolver::query query(
    url.host, 
    determine_service_v2(url));

  resolver_.async_resolve( query,
    boost::bind(&agent_base_v2::handle_resolve, this, _1, _2, ctx));
}

void agent_base_v2::handle_resolve(
  boost::system::error_code const &err, 
  tcp::resolver::iterator endpoint,
  context_ptr ctx_ptr)
{
  sys::error_code internal_err;

  if(!err && endpoint != tcp::resolver::iterator() ) {
    // test whether to reconnect
    if( connection_ && 
        connection_->is_open() &&
        endpoint->endpoint() ==
        connection_->socket().remote_endpoint(internal_err))
    {
      AGENT_TRACKING("agent_base::handle_resolve(reuse) " + 
                     endpoint->endpoint().address().to_string());
      if(!internal_err) {
        // XXX This is sufficient for Linux, Windows requires further
        // exploiration
        handle_connect(err, ctx_ptr);
        return;
      }
    }
    AGENT_TRACKING("agent_base_v2::handle_resolve(reconnect) " + 
                   endpoint->endpoint().address().to_string());
    connection_.reset(new connection(io_service_));
    ctx_ptr->session->connect_handler =
      boost::bind(&agent_base_v2::handle_connect, this, _1, ctx_ptr);
    connection_->connect(endpoint, *(ctx_ptr->session));
  }else {
    notify_error(err, ctx_ptr);
  }
}

void agent_base_v2::handle_connect(
  boost::system::error_code const &err,
  context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::handle_connect");
  if(!err) {
#ifdef AGENT_LOG_HEADERS
    boost::system::error_code error;
    logger::instance().async_log(
      "request headers", true, 
      connection_->socket().remote_endpoint(error),
      ctx_ptr->request);
#endif
    std::ostream os(&ctx_ptr->session->io_buffer);
    os << ctx_ptr->request;
    // Remember request size (include headers)
    // XXX expected_size is used both in upstream and downstream but in
    // different maner. That's tricky.
    ctx_ptr->expected_size = ctx_ptr->session->io_buffer.size();
    if(ctx_ptr->mpart) 
      ctx_ptr->expected_size += ctx_ptr->mpart->size();
    handle_write_request(err, 0, ctx_ptr);
  } else {
    notify_error(err, ctx_ptr);
  }
}

boost::asio::io_service &agent_base_v2::io_service()
{
  return io_service_;
}

void agent_base_v2::handle_write_request(
  boost::system::error_code const &err,
  boost::uint32_t length,
  context_ptr ctx_ptr)
{
  AGENT_TRACKING_2("agent_base_v2::handle_write_request", length);
  if(!err) {
    ctx_ptr->session->io_buffer.consume(length);
    // read data from multipart 
    if(ctx_ptr->mpart && !ctx_ptr->mpart->eof())
      ctx_ptr->mpart->read(ctx_ptr->session->io_buffer, 4096);
    if( length ) {
      notify_monitor(ctx_ptr, 
                     agent_conn_action_t::upstream, 
                     length);
    }
    if( ctx_ptr->session->io_buffer.size() ) { // more data to send
      ctx_ptr->session->io_handler = boost::bind(
        &agent_base_v2::handle_write_request, this,
        asio_ph::error, asio_ph::bytes_transferred, ctx_ptr);
      connection_->write(*ctx_ptr->session);
    } else {
      // Clear expected_size since it will be used when recesiving data
      ctx_ptr->expected_size = 0;
      ctx_ptr->session->io_handler = boost::bind(
        &agent_base_v2::handle_read_status_line, this, _1, ctx_ptr);
      connection_->read_until("\r\n", 2048, *ctx_ptr->session);
    }
  } else {
    notify_error(err, ctx_ptr);
  }
}

void agent_base_v2::handle_read_status_line(
  const boost::system::error_code& err,
  context_ptr ctx_ptr)
{
  using http::parser::parse_response_first_line;

  AGENT_TRACKING("agent_base_v2::handle_read_status_line");
  if (!err) {
    // Check that response is OK.
    auto beg(asio::buffers_begin(ctx_ptr->session->io_buffer.data())), 
         end(asio::buffers_end(ctx_ptr->session->io_buffer.data()));
    if(!parse_response_first_line(beg, end, ctx_ptr->response)) {
      sys::error_code ec(sys::errc::bad_message, 
                         sys::system_category());
      notify_error(ec, ctx_ptr);
      return;
    }
    ctx_ptr->session->io_buffer.consume(
      beg - asio::buffers_begin(ctx_ptr->session->io_buffer.data()));
    ctx_ptr->session->io_handler = boost::bind(
      &agent_base_v2::handle_read_headers, this, _1, ctx_ptr);
    // Read the response headers, which are terminated by a blank line.
    connection_->read_until("\r\n\r\n", 4096, *ctx_ptr->session);
  } else {
    notify_error(err, ctx_ptr);
  }
}

void agent_base_v2::handle_read_headers(const boost::system::error_code& err,
                                        context_ptr ctx_ptr)
{
  using http::parser::parse_header_list;

  AGENT_TRACKING("agent_base_v2::handle_read_headers");
  if (!err) {
    // Process the response headers.
    auto beg(asio::buffers_begin(ctx_ptr->session->io_buffer.data())), 
         end(asio::buffers_end(ctx_ptr->session->io_buffer.data())),
         iter(beg);

    if(!parse_header_list(iter, end, ctx_ptr->response.headers)) {
      sys::error_code ec(sys::errc::bad_message, 
                         sys::system_category());
      notify_error(ec, ctx_ptr);
      return;
    }
#ifdef AGENT_LOG_HEADERS
    sys::error_code internal_err;
    logger::instance().async_log(
      "response headers", true, 
      connection_->socket().remote_endpoint(internal_err),
      ctx_ptr->response);
#endif
    ctx_ptr->session->io_buffer.consume(iter - beg);
    diagnose_transmission(ctx_ptr);
  } else {
    notify_error(err, ctx_ptr);
  }
}

void agent_base_v2::diagnose_transmission(context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::diagnose_transmission");

#define FIND_HEADER_(Header) \
  http::find_header(ctx_ptr->response.headers, Header)
  auto npos = ctx_ptr->response.headers.end();
  decltype(npos) header;
  ctx_ptr->expected_size = 0;
  ctx_ptr->is_redirecting = 
    ctx_ptr->response.status_code >= 300 && 
    ctx_ptr->response.status_code < 400;

  // TODO Content-Range handling
  
  if(npos != (header = FIND_HEADER_("Transfer-Encoding"))) {
    sys::error_code ec;
    if( !ctx_ptr->chunked_callback ) {
      // XXX Sorry for this.
      ec.assign(sys::errc::not_supported, sys::system_category());
      notify_error(ec, ctx_ptr);
    } else {
      read_chunk(ctx_ptr);
    }
  } else { // no transfer encoding field
    if(npos != (header = FIND_HEADER_("Content-Length"))) {
      ctx_ptr->expected_size = header->value_as<boost::intmax_t>();
    } else { // no content length
      if( npos != (header = FIND_HEADER_("Connection")) &&
          header->value == "close")
        ctx_ptr->expected_size = (boost::uintmax_t)-1;
      else
        assert(false && "Unable to receive body.");
    }
    read_body(ctx_ptr);
  }
}

void agent_base_v2::read_chunk(context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::read_chunk");
  if( ctx_ptr->session->io_buffer.size() <= 3) { // 3 bytes for "0\r\n"
    ctx_ptr->session->io_handler = boost::bind(
      &agent_base_v2::handle_read_chunk, this, asio_ph::error, ctx_ptr);
    connection_->read_some(1, *ctx_ptr->session);
  } else {
    sys::error_code ec;
    handle_read_chunk(ec, ctx_ptr);
  }
}

void agent_base_v2::handle_read_chunk(
  boost::system::error_code const& err,
  context_ptr ctx_ptr)
{
  using boost::lexical_cast;
  using boost::numeric_cast;
  using boost::intmax_t;
  using namespace std;
  
  AGENT_TRACKING("agent_base_v2::handle_read_chunk");
  if(!err) {
    while(ctx_ptr->session->io_buffer.size() ) {
      if( ctx_ptr->expected_size ) {
        size_t to_write;
        try {
          to_write = min(
            ctx_ptr->session->io_buffer.size(), 
            numeric_cast<size_t>(ctx_ptr->expected_size));
        } catch (boost::bad_numeric_cast &) {
          to_write = ctx_ptr->session->io_buffer.size();
        }
        notify_chunk(err, to_write, ctx_ptr);
        ctx_ptr->expected_size -= to_write;
      } else {
        auto 
          beg(asio::buffers_begin(ctx_ptr->session->io_buffer.data())), 
          end(asio::buffers_end(ctx_ptr->session->io_buffer.data()));
        auto origin = beg, pos = beg;
        char const* crlf("\r\n");
        while( end != (pos = search(beg, end, crlf, crlf + 2))) {
          if( pos == beg) {
            beg = pos + 2;
          } else {
            if( '0' == *beg) {
              ctx_ptr->session->io_buffer.consume(3);
              notify_error(asio::error::eof, ctx_ptr);
              return;
            }
            ctx_ptr->expected_size = strtol(&*beg, NULL, 16);
            ctx_ptr->session->io_buffer.consume((pos + 2) - origin);
            if(!ctx_ptr->expected_size) {
              sys::error_code ec(sys::errc::bad_message, 
                                 sys::system_category());
              notify_error(ec, ctx_ptr);
              return;
            }
            break;
          }
        } // while crlf is matched
        if(!ctx_ptr->expected_size)
          break;
      }
    } // while there are buffered data
    read_chunk(ctx_ptr);
  } else {
    // XXX This case is caused by user manually abort
    // agent operation.
    if( connection_ && !connection_->is_open() && 
        err == asio::error::bad_descriptor ) 
    {
      notify_error(asio::error::operation_aborted, ctx_ptr);
    } else {
      notify_error(err, ctx_ptr);
    }
  }
}

void agent_base_v2::read_body(context_ptr ctx_ptr) 
{
  AGENT_TRACKING("agent_base_v2::read_body");
  if(!ctx_ptr->session->io_buffer.size() && ctx_ptr->expected_size ) {
    ctx_ptr->session->io_handler = 
      boost::bind(&agent_base_v2::handle_read_body, this, _1, _2, ctx_ptr);
    connection_->read_some(1, *ctx_ptr->session);
  } else {
    sys::error_code ec;
    handle_read_body(ec, ctx_ptr->session->io_buffer.size(), ctx_ptr);
  }
}

void agent_base_v2::handle_read_body(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::handle_read_body");
  ctx_ptr->expected_size -= length;
  if (!err && ctx_ptr->expected_size) {
    if( ctx_ptr->chunked_callback ) 
      notify_chunk(err, -1, ctx_ptr);
    boost::uint32_t delay = estimate_delay(ctx_ptr, downstream, length);
    if(!delay) {
      read_body(ctx_ptr);
    } else {
      AGENT_TRACKING_2("agent_base_v2::handle_read_body(delay)", delay);
      ctx_ptr->session->timer.expires_from_now(boost::posix_time::seconds(delay));
      ctx_ptr->session->timer.async_wait(boost::bind(&agent_base_v2::read_body, this, ctx_ptr));
    }
  } else {
    if( err == asio::error::eof || 0 == ctx_ptr->expected_size ) {
      notify_error(asio::error::eof, ctx_ptr);
    } 
    // XXX This case is caused by user manually abort
    // agent operation.
    else if( connection_ && !connection_->is_open() && 
             err == asio::error::bad_descriptor) 
    {
      notify_error(asio::error::operation_aborted, ctx_ptr);
    } 
    else {
      notify_error(err, ctx_ptr);
    }
  }
}

void agent_base_v2::redirect(context_ptr ctx_ptr)
{
  namespace sys = boost::system;
  sys::error_code http_err;
  auto location = 
    http::find_header(ctx_ptr->response.headers, "Location"); 
  auto connect_policy = 
    http::find_header(ctx_ptr->request.headers, "Connection");
  auto beg(location->value.begin()), end(location->value.end());

  if(AGENT_MAXIMUM_REDIRECT_COUNT <= ctx_ptr->redirect_count)
    goto OPERATION_CANCEL;
  if(location == ctx_ptr->response.headers.end())
    goto BAD_MESSAGE;

  AGENT_TRACKING("agent_base_v2::redirect");

  ctx_ptr->redirect_count++;
  if( location->value.find("http://") == 0  ||
      location->value.find("https://") == 0 ) 
  {
    if( connect_policy->value == "close" )
      connection_.reset();
    http::entity::url url(location->value);
    async_request(url, ctx_ptr->request, ctx_ptr->request.method, ctx_ptr->chunked_callback, 
              ctx_ptr->handler, ctx_ptr->monitor);
  } else { // we got a non-standard relative redirection that sucks!
    if(location->value[0] != '/')
      location->value.insert(0, "/");
    std::stringstream cvt;
    cvt << "http://" << 
      connection_->socket().remote_endpoint(http_err) << 
      location->value;
    if(http_err) {
      ctx_ptr->is_redirecting = false;
      notify_error(http_err, ctx_ptr);
    } else {
      if( connect_policy->value == "close" )
        connection_.reset();
      http::entity::url url(cvt.str());
      async_request(url, ctx_ptr->request, ctx_ptr->request.method, ctx_ptr->chunked_callback, 
                ctx_ptr->handler, ctx_ptr->monitor);
    }
  }
  return;
BAD_MESSAGE:
  ctx_ptr->is_redirecting = false;
  http_err.assign(sys::errc::bad_message, sys::system_category());
  notify_error(http_err, ctx_ptr);
  return;
OPERATION_CANCEL:
  ctx_ptr->is_redirecting = false;
  http_err.assign(sys::errc::operation_canceled, sys::system_category());
  notify_error(http_err, ctx_ptr);
  return;
}

void agent_base_v2::notify_chunk(
  boost::system::error_code const &err, 
  boost::uint32_t length,
  context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::notify_chunk");
  char const* data = asio::buffer_cast<char const*>(
    ctx_ptr->session->io_buffer.data());
  boost::uint32_t size = std::min(
    (size_t)length, ctx_ptr->session->io_buffer.size());
  if(!ctx_ptr->is_redirecting) {
    asio::const_buffer chunk(data,size);
    ctx_ptr->handler(err, ctx_ptr->request, ctx_ptr->response, chunk, ctx_ptr->qos);
    notify_monitor(ctx_ptr,
                   agent_conn_action_t::downstream,
                   size);
  }
  ctx_ptr->session->io_buffer.consume(size);
}

void agent_base_v2::notify_error(boost::system::error_code const &err,
                                 context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base_v2::notify_error (" + err.message() + ")");
  boost::system::error_code ec(err);
  if(ctx_ptr->is_redirecting) {
    redirect(ctx_ptr);
    ctx_ptr->response.clear();
  } else {
    char const *data = asio::buffer_cast<char const*>(ctx_ptr->session->io_buffer.data());
    auto size = ctx_ptr->session->io_buffer.size();
    asio::const_buffer chunk(data, size);
    ctx_ptr->handler(ec, ctx_ptr->request, ctx_ptr->response, chunk, ctx_ptr->qos);
    notify_monitor(ctx_ptr,
                   agent_conn_action_t::downstream,
                   size);
    auto connect_policy = http::find_header(ctx_ptr->response.headers, "Connection");
    
    if( connect_policy != ctx_ptr->response.headers.end() &&
        connect_policy->value == "close" )
      connection_.reset();

    if( err != asio::error::eof )
      connection_.reset();
          
    ctx_ptr.reset();
  }
}

void agent_base_v2::notify_monitor(
  context_ptr ctx_ptr,
  agent_conn_action_t action, 
  boost::uint32_t transfered)
{
  AGENT_TRACKING_2("agent_base_v2::notify_monitor", transfered);

  if(ctx_ptr->monitor) {
    
    boost::uintmax_t total = 0;
    if(action == agent_conn_action_t::upstream) {
      total = ctx_ptr->expected_size;
    } else {
      auto content_length = http::find_header(
        ctx_ptr->response.headers, "Content-Length");
      if( content_length != ctx_ptr->response.headers.end() ) {
        total = content_length->value_as<boost::uintmax_t>();
      }
    }
    
    io_service().post(boost::bind(
        ctx_ptr->monitor, action, total, transfered));
  }
}

boost::uint32_t agent_base_v2::estimate_delay(
  context_ptr ctx_ptr,
  agent_conn_action_t action, 
  boost::uint32_t transfered)
{
  using boost::uint32_t;
  using std::time;
  
  uint32_t delay_seconds = 0;
  if(action == upstream) {
    if( ctx_ptr->qos.write_max_bps != quality_config::unlimited) {
       // TODO Upstream limit
    }
  } else { // downstream
    if( ctx_ptr->qos.read_max_bps != quality_config::unlimited) {
      ctx_ptr->receive_since_last_limit_check += transfered;
      if(ctx_ptr->receive_since_last_limit_check > (boost::uint32_t)ctx_ptr->qos.read_max_bps) {
        // XXX Here I assume each async_read will return in 1 second
       delay_seconds = ctx_ptr->receive_since_last_limit_check / ctx_ptr->qos.read_max_bps;
       ctx_ptr->receive_since_last_limit_check = 0;
      }
    }
  }
  AGENT_TRACKING_2("agent_base_v2::estimate_delay", delay_seconds);
  return delay_seconds;
}

