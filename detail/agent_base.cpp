#include "agent/agent_base.hpp"
#include <string>
#include <sstream>
#include <cstring>
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

#define USER_AGENT_STR "OokonHTTPAgent Version"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#else
#   define AGENT_TRACKING(Desc)
#endif

std::string 
determine_service(http::entity::url const & url)
{
  return (url.port) ?
    boost::lexical_cast<std::string>(url.port) :
    url.scheme
    ;
}

void
setup_default_headers(std::vector<http::entity::field> &headers)
{
  auto header = http::get_header(headers, "Accept");
  if(header->value.empty()) header->value = "*/*";
  header = http::get_header(headers, "Connection");
  if(header->value.empty()) header->value = "keep-alive";
  header = http::get_header(headers, "User-Agent");
  if(header->value.empty()){
    std::string ustr = USER_AGENT_STR;
    ustr += "/";
    ustr += agent_version();
    header->value = ustr;
  }
}

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

agent_base::agent_base(asio::io_service &io_service)
: io_service_(io_service),
  resolver_(io_service),
  redirect_count_(0), 
  chunked_callback_(false)
{
  AGENT_TRACKING("agent_base::ctor");
}

agent_base::~agent_base()
{
  AGENT_TRACKING("agent_base::dtor");
}

void agent_base::async_get(http::entity::url const& url, bool chunked_callback,
                           handler_type handler)
{
  init("GET", url);
  std::ostream os(&session_->io_buffer);

  chunked_callback_ = chunked_callback;
  os.flush();
  os << request_;
  start_op(url.host, determine_service(url), handler);
}


void agent_base::async_get(http::entity::url const &url, bool chunked_callback,
                           boost::uint64_t offset, boost::uint64_t size,
                           handler_type handler)
{
  using http::entity::field;

  init("GET", url);
  std::ostream os(&session_->io_buffer);
  std::stringstream range;
  range << "bytes=";
  if( offset != -1) range << offset;
  range << "-";
  if(size != -1 ) range << (offset + size -1);

  chunked_callback_ = chunked_callback;
  request_.headers << field("Range", range.str());
  os.flush();
  os << request_;
  start_op(url.host, determine_service(url), handler);

}

void agent_base::async_post(http::entity::url const &url, 
                http::entity::query_map_t const &post_parameter,
                bool chunked_callback,
                handler_type handler)
{
  using namespace http;

  init("POST", url);
  std::ostream os(&session_->io_buffer);
  std::stringstream body;
  generator::ostream_iterator body_begin(body);
  auto content_type = 
    get_header(request_.headers, "Content-Type");
  auto content_length = 
    get_header(request_.headers, "Content-Length");

  if(content_type->value.empty()) content_type->value = 
    "application/x-www-form-urlencoded";

  generator::generate_query_map(body_begin, post_parameter);
  content_length->value_as(body.str().size());

  chunked_callback_ = chunked_callback;
  os.flush();
  os << request_ << body.str();
  start_op(url.host, determine_service(url), handler);
}

void agent_base::async_post_file(http::entity::url const &url,
                     http::entity::query_map_t const &post_parameter,
                     std::string const &filename,
                     handler_type handler)
{
  char const *CRLF = "\r\n";

  /*
  init("POST", url);
  std::ostream os(&session_->io_buffer);
  std::stringstream body;
  auto content_type = 
    get_header(request_.headers, "Content-Type");
  auto content_length = 
    get_header(request_.headers, "Content-Length");
  std::string boundary = "----AgentBoundaryASDFGHJKL--";
  // FIXME make boundary randomize
  content_type->value = "multipart/form-data; boundary=" + boundary;

  for(auto i = post_parameter.begin(); i != post_parameter.end(); ++i) {
    / * TODO
    body << "--" << boundary << CRLF <<
      "Content-Disposition: form-data; name=\"" << i->first;
      * /
  }
  chunked_callback_ = false;
  */

}

void agent_base::async_abort()
{
  if(connection_ && connection_->is_open()) {
    AGENT_TRACKING("agent_base::async_abort");
    connection_->close();
  }
}

http::request &agent_base::request()
{  return request_;  }

asio::io_service &agent_base::io_service()
{ return io_service_; }

void
agent_base::init(std::string const &method, http::entity::url const &url) 
{
  auto host = http::get_header(request_.headers, "Host");

  if(request_.query.path.empty())
    request_.query.path = "/";
  request_.method = method;
  request_.http_version_major = 1;
  request_.http_version_minor = 1;
  request_.query = url.query;
  host->value = url.host;
  if(url.port) 
    host->value += ":" + boost::lexical_cast<std::string>(url.port);
  setup_default_headers(request_.headers);

  redirect_count_ = 0;
  session_.reset(new session_type(io_service_));
}

void agent_base::start_op(
  std::string const &server, std::string const &port, handler_type handler)
{
  AGENT_TRACKING("agent_base::start_op");

  // TODO global management of connection
  is_redirecting_ = false;
  response_.clear();
  handler_.reset(new handler_type(handler));

  tcp::resolver::query query(server, port);
  resolver_.async_resolve(
    query, 
    boost::bind(&agent_base::handle_resolve, this, _1, _2));
}

void agent_base::handle_resolve(boost::system::error_code const &err, 
                    tcp::resolver::iterator endpoint)
{

  if(!err && endpoint != tcp::resolver::iterator()) {
    // XXX The is_open does not react as expected
    // is the same server:port ?
    sys::error_code err_;
    if( connection_ && connection_->is_open() && 
        endpoint->endpoint() == connection_->socket().remote_endpoint(err_))
    {
      AGENT_TRACKING("agent_base::handle_resolve(reuse) " + endpoint->endpoint().address().to_string());
      if(!err_)
        write_request();
      else
        notify_error(err_);
    } else {
      AGENT_TRACKING("agent_base::handle_resolve(reconnect) " + endpoint->endpoint().address().to_string());
      connection_.reset(new connection(io_service_));
      session_->connect_handler = 
        boost::bind(&agent_base::handle_connect, this, asio_ph::error);
      connection_->connect(endpoint, *session_);
    }
  } else {
    notify_error(err);
  }
}

void agent_base::handle_connect(boost::system::error_code const &err)
{
  AGENT_TRACKING("agent_base::handle_connect");
  if(!err) {
    write_request();
  } else {
    notify_error(err);
  }
}

void agent_base::write_request()
{
  AGENT_TRACKING("agent_base::write_request");
  sys::error_code error;
#ifdef AGENT_LOG_HEADERS
  logger::instance().async_log(
    "request headers", true, 
    connection_->socket().remote_endpoint(error),
    request_);
#endif
  if( error ) {
    notify_error(error);
  } else {
    session_->io_handler = boost::bind(
      &agent_base::handle_write_request, this,
      asio_ph::error,
      asio_ph::bytes_transferred);
    connection_->write(*session_); 
  }
}

void agent_base::handle_write_request(
  boost::system::error_code const &err, 
  boost::uint32_t len)
{
  // FIXME  0 buffer
  AGENT_TRACKING("agent_base::handle_write_request");
  if (!err ){
    session_->io_buffer.consume(len);
    assert(session_->io_buffer.size() == 0);
    session_->io_handler = boost::bind(
      &agent_base::handle_read_status_line, this,
      asio_ph::error);
    connection_->read_until("\r\n", 512, *session_);
  } else {
    notify_error(err);
  }
}

void agent_base::handle_read_status_line(const boost::system::error_code& err)
{
  AGENT_TRACKING("agent_base::handle_read_status_line");
  if (!err) {
    sys::error_code http_err;
    // Check that response is OK.
    auto beg(asio::buffers_begin(session_->io_buffer.data())), 
         end(asio::buffers_end(session_->io_buffer.data()));
    if(!http::parser::parse_response_first_line(beg, end, response_)){
      http_err.assign(sys::errc::bad_message, sys::system_category());
      notify_error(http_err);
      return;
    }
    session_->io_buffer.consume(
      beg - asio::buffers_begin(session_->io_buffer.data()));
    session_->io_handler = boost::bind(
      &agent_base::handle_read_headers, this, asio_ph::error);
    // Read the response headers, which are terminated by a blank line.
    connection_->read_until("\r\n\r\n", 4096, *session_);
  } else {
    notify_error(err);
  }
}

void agent_base::handle_read_headers(const boost::system::error_code& err)
{
  AGENT_TRACKING("agent_base::handle_read_headers");
  if (!err) {
    // Process the response headers.
    auto beg(asio::buffers_begin(session_->io_buffer.data())), 
         end(asio::buffers_end(session_->io_buffer.data()));
    sys::error_code http_err;
    if(!http::parser::parse_header_list(beg, end, response_.headers)) {
      http_err.assign(sys::errc::bad_message, sys::system_category());
      notify_error(http_err);
      return;
    }
#ifdef AGENT_LOG_HEADERS
    logger::instance().async_log(
      "response headers", true, 
      connection_->socket().remote_endpoint(http_err),
      response_);
#endif
    if(http_err) {
      notify_error(http_err); 
    } else {
      session_->io_buffer.consume(
        beg - asio::buffers_begin(session_->io_buffer.data()));
      diagnose_transmission();
    }
  } else {
    notify_error(err);
  }
}

void agent_base::diagnose_transmission()
{
  AGENT_TRACKING("agent_base::diagnose_transmission");

#define FIND_HEADER_(Header) \
  http::find_header(response_.headers, Header)
  auto npos = response_.headers.end();
  decltype(npos) header;
  expected_size_ = 0;
  is_redirecting_ = 
    response_.status_code >= 300 && response_.status_code < 400;

  // TODO Content-Range handling
  
  if(npos != (header = FIND_HEADER_("Transfer-Encoding"))) {
    if( !chunked_callback_ ) {
      // XXX Sorry for this.
      notify_error(sys::error_code(sys::errc::not_supported, sys::system_category()));
    } else {
      notify_header(sys::error_code());
      read_chunk();
    }
  } else { 
    if(npos != (header = FIND_HEADER_("Content-Length"))) {
      expected_size_ = header->value_as<boost::int64_t>();
    } else {
      if( npos != (header = FIND_HEADER_("Connection")) &&
          header->value == "close")
        expected_size_ = std::numeric_limits<boost::uint64_t>::max();
    }
    assert(expected_size_ >= session_->io_buffer.size());
    expected_size_ -= session_->io_buffer.size();
    notify_header(sys::error_code());
    if( expected_size_ ) {
      read_body();
    } else {
      notify_error(asio::error::eof);
    }
  }
}

void agent_base::read_chunk()
{
  AGENT_TRACKING("agent_base::read_chunk");
  session_->io_handler = 
    boost::bind(&agent_base::handle_read_chunk, this, asio_ph::error);
  connection_->read_some(1, *session_);
}

void agent_base::handle_read_chunk(boost::system::error_code const& err)
{
  using boost::lexical_cast;
  using boost::numeric_cast;
  using boost::int64_t;
  using namespace std;
  
  AGENT_TRACKING("agent_base::handle_read_chunk");
  if(!err) {
    while(session_->io_buffer.size() ) {
      if( expected_size_ ) {
        size_t to_write;
        try {
          to_write = min(
            session_->io_buffer.size(), numeric_cast<size_t>(expected_size_));
        } catch (boost::bad_numeric_cast &) {
          to_write = session_->io_buffer.size();
        }
        notify_chunk(err, to_write);
        expected_size_ -= to_write;
      } else {
        auto 
          beg(asio::buffers_begin(session_->io_buffer.data())), 
          end(asio::buffers_end(session_->io_buffer.data()));
        auto origin = beg, pos = beg;
        char const* crlf("\r\n");
        while( end != (pos = search(beg, end, crlf, crlf + 2))) {
          if( pos == beg) {
            beg = pos + 2;
          } else {
            if( '0' == *beg) {
              notify_error(asio::error::eof);
              return;
            }
            expected_size_ = strtol(&*beg, NULL, 16);
            session_->io_buffer.consume((pos + 2) - origin);
            if(!expected_size_) {
              notify_error(sys::error_code(sys::errc::bad_message, sys::system_category()));
              return;
            }
            break;
          }
        } // while crlf is matched
        if(!expected_size_ )
          break;
      }
    } // while there are buffered data
    read_chunk();
  } else {
    // XXX This case is caused by user manually abort
    // agent operation.
    if( connection_ && !connection_->is_open() && 
        err == asio::error::bad_descriptor ) 
    {
      notify_error(asio::error::operation_aborted);
    } else {
      notify_error(err);
    }
  }
}

void agent_base::read_body() 
{
  AGENT_TRACKING("agent_base::read_body");
  session_->io_handler = 
    boost::bind(&agent_base::handle_read_body, this, _1, _2);
  connection_->read_some(1, *session_);
}

void agent_base::handle_read_body(
  boost::system::error_code const &err, boost::uint32_t length)
{
  AGENT_TRACKING("agent_base::handle_read_body");
  expected_size_ -= length;
  if (!err && expected_size_) {
    if( chunked_callback_) 
      notify_chunk(err);
    read_body();
  } else {
    if( err == asio::error::eof || 0 == expected_size_ ) {
      notify_error(asio::error::eof);
    } 
    // XXX This case is caused by user manually abort
    // agent operation.
    else if( connection_ && !connection_->is_open() && 
             err == asio::error::bad_descriptor) 
    {
      notify_error(asio::error::operation_aborted);
    } 
    else {
      notify_error(err);
    }
  }
}

void agent_base::redirect()
{
  namespace sys = boost::system;
  sys::error_code http_err;
  http::entity::url url;
  auto location = http::find_header(response_.headers, "Location"); 
  auto connect_policy = http::find_header(request_.headers, "Connection");
  auto beg(location->value.begin()), end(location->value.end());

  if(AGENT_MAXIMUM_REDIRECT_COUNT <= redirect_count_)
    goto OPERATION_CANCEL;
  if(location == response_.headers.end())
    goto BAD_MESSAGE;

  AGENT_TRACKING("agent_base::redirect");

  redirect_count_++;
  if( location->value.find("http://") == 0  ||
      location->value.find("https://") == 0 ) 
  {
    if( connect_policy->value == "close" )
      connection_.reset();
    async_get(location->value, chunked_callback_,  *handler_);
  } else { // we got a non-standard relative redirection that sucks!
    if(location->value[0] != '/')
      location->value.insert(0, "/");
    std::stringstream cvt;
    cvt << "http://" << 
      connection_->socket().remote_endpoint(http_err) << 
      location->value  ;
    if(http_err) {
      is_redirecting_ = false;
      notify_error(http_err);
    } else {
      if( connect_policy->value == "close" )
        connection_.reset();
      async_get(cvt.str(), chunked_callback_, *handler_);
    }
  }
  return;

  BAD_MESSAGE:
    is_redirecting_ = false;
    http_err.assign(sys::errc::bad_message, sys::system_category());
  notify_error(http_err);
  return;

  OPERATION_CANCEL:
    is_redirecting_ = false;
    http_err.assign(sys::errc::operation_canceled, sys::system_category()); 
  notify_error(http_err);
  return;
}

void agent_base::notify_header(boost::system::error_code const &err)
{
  AGENT_TRACKING("agent_base::notify_header");
  if(!is_redirecting_) {
    (*handler_)(err, request_, response_, asio::const_buffer());
  }
}

void agent_base::notify_chunk(boost::system::error_code const &err, boost::uint32_t length)
{
  AGENT_TRACKING("agent_base::notify_chunk");
  char const* data = asio::buffer_cast<char const*>(session_->io_buffer.data());
  boost::uint32_t size = std::min((size_t)length, session_->io_buffer.size());
  if(!is_redirecting_) {
    asio::const_buffer chunk(data,size);
    (*handler_)(err, request_, response_, chunk);
  }
  session_->io_buffer.consume(size);
}

void agent_base::notify_error(boost::system::error_code const &err)
{
  AGENT_TRACKING("agent_base::notify_error");
  auto connect_policy = http::find_header(request_.headers, "Connection");
  
  if(!is_redirecting_) {
    char const* data = asio::buffer_cast<char const*>(session_->io_buffer.data());
    asio::const_buffer chunk(data, session_->io_buffer.size());
    (*handler_)(err, request_, response_, chunk);
    if( connect_policy->value == "close" )
      connection_.reset();
    session_.reset();
    request_.clear();
    response_.clear();
    handler_.reset();
  } else {
    redirect(); 
    response_.clear();
  } 
}

