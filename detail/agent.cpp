#include "agent/agent.hpp"
#include <string>
#include <sstream>
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
#include "log.hpp"

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
  if(header->value.empty()) header->value = "close";
  header = http::get_header(headers, "User-Agent");
  if(header->value.empty()) header->value = "agent/http";
}

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;
using boost::bind;

agent::agent(asio::io_service &ios)
: io_service_(ios), 
  connection_(new connection(ios)),
  redirect_count_(0), 
  chunked_callback_(false)
{}

agent::~agent()
{}

void agent::get(std::string const &url, 
                  http::entity::query_map_t const &parameter,
                  bool chunked_callback, 
                  handler_type handler)
{
  sys::error_code http_err;
  http::entity::url url_;
  std::ostream os(&connection_->io_buffer());
  auto beg(url.begin()), end(url.end());

  if(!http::parser::parse_url(beg, end, url_)) {
    http_err.assign(sys::errc::invalid_argument, sys::system_category());
    notify_error(http_err);
    return;
  }
  request_.method = "GET";
  request_.http_version_major = 1;
  request_.http_version_minor = 1;
  auto header = http::get_header(request_.headers, "Host");
  if(header->value.empty()) header->value = url_.host;
  setup_default_headers(request_.headers);
  request_.query.path = url_.query.path;
  request_.query.query_map = parameter;

  redirect_count_ = 0;
  chunked_callback_ = chunked_callback;
  os.flush();
  os << request_;
  start_op(url_.host, determine_service(url_), handler);
}

void agent::post(std::string const &url,
          http::entity::query_map_t const &get_parameter,
          http::entity::query_map_t const &post_parameter,
          bool chunked_callback, 
          handler_type handler)
{
  sys::error_code http_err;
  http::entity::url url_;
  std::ostream os(&connection_->io_buffer());
  std::stringstream body;
  auto beg(url.begin()), end(url.end());
  http::generator::ostream_iterator body_begin(body);

  if(!http::parser::parse_url(beg, end, url_)) {
    http_err.assign(sys::errc::invalid_argument, sys::system_category());
    notify_error(http_err);
    return;
  }
  request_.method = "POST";
  request_.http_version_major = 1;
  request_.http_version_minor = 1;
  auto header = http::get_header(request_.headers, "Host");
  if(header->value.empty()) header->value = url_.host;
  header = http::get_header(request_.headers, "Content-Type");
  if(header->value.empty()) header->value = 
    "application/x-www-form-urlencoded";
  setup_default_headers(request_.headers);
  request_.query.path = url_.query.path;
  request_.query.query_map = get_parameter;
  http::generator::generate_query_map(body_begin, post_parameter);
  header = http::get_header(request_.headers, "Content-Length");
  header->value_as(body.str().size());

  redirect_count_ = 0;
  chunked_callback_ = chunked_callback;
  os.flush();
  os << request_ << body.str();
  start_op(url_.host, determine_service(url_), handler);
}

http::request &agent::request()
{  return request_;  }

void agent::start_op(
  std::string const &server, std::string const &port, handler_type handler)
{
  // TODO global management of connection
  handler_ = handler;
  connection_->connect(
    server, port,
    bind(
      &agent::handle_connect, this, asio_ph::error));
}

void agent::handle_connect(boost::system::error_code const &err)
{
  if(!err) {
#ifdef AGENT_LOG_HEADERS
    AGENT_TIMED_LOG("request headers", request_);
#endif
    connection_->write(
      bind(
        &agent::handle_write_request, this,
        asio_ph::error,
        asio_ph::bytes_transferred
        ));
  } else {
    notify_error(err);
  }
}

void agent::handle_write_request(
  boost::system::error_code const &err, 
  boost::uint32_t len)
{
  // FIXME  0 buffer
  if (!err ){
    connection_->io_buffer().consume(len);
    connection_->read_until(
      "\r\n", bind(
        &agent::handle_read_status_line, this,
        asio_ph::error));
  } else {
    notify_error(err);
  }
}

void agent::handle_read_status_line(const boost::system::error_code& err)
{
  if (!err) {
    sys::error_code http_err;
    // Check that response is OK.
    auto beg(asio::buffers_begin(connection_->io_buffer().data())), 
         end(asio::buffers_end(connection_->io_buffer().data()));

    if(!http::parser::parse_response_first_line(beg, end, response_)){
      http_err.assign(sys::errc::bad_message, sys::system_category());
      notify_error(http_err);
      return;
    }
    connection_->io_buffer().consume(
      beg - asio::buffers_begin(connection_->io_buffer().data()));
    // Read the response headers, which are terminated by a blank line.
    connection_->read_until(
      "\r\n\r\n", 
      bind(
        &agent::handle_read_headers, this,
        asio_ph::error));
  } else {
    notify_error(err);
  }
}

void agent::handle_read_headers(const boost::system::error_code& err)
{
  if (!err) {
    // Process the response headers.
    auto beg(asio::buffers_begin(connection_->io_buffer().data())), 
         end(asio::buffers_end(connection_->io_buffer().data()));

    if(!http::parser::parse_header_list(beg, end, response_.headers)) {
      sys::error_code http_err(sys::errc::bad_message, sys::system_category());
      notify_error(http_err);
      return;
    }
#ifdef AGENT_LOG_HEADERS
    AGENT_TIMED_LOG("response headers", response_);
#endif
    connection_->io_buffer().consume(
      beg - asio::buffers_begin(connection_->io_buffer().data()));
    // Handle redirection - i.e. 301, 302, 
    if(response_.status_code >= 300 && response_.status_code < 400){
      redirect();
    } else {
      notify_header(err);
      read_body();
    }
  } else {
    notify_error(err);
  }
}

void agent::read_body() 
{
  connection_->read_some(
    1, bind(&agent::handle_read_body, this, _1, _2));
}

void agent::handle_read_body(
  boost::system::error_code const &err, boost::uint32_t length)
{
  if (!err ) {
    if( chunked_callback_) 
      notify_chunk(err);
    read_body();
  } else {
    notify_error(err);
  }

}

void agent::redirect()
{
  namespace sys = boost::system;
  sys::error_code http_err;

  http::entity::url url;
  auto iter = http::find_header(response_.headers, "Location"); 
  auto beg(iter->value.begin()), end(iter->value.end());

  if(AGENT_MAXIMUM_REDIRECT_COUNT <= redirect_count_)
    goto OPERATION_CANCEL;
  if(iter == response_.headers.end())
    goto BAD_MESSAGE;
  if(!http::parser::parse_url(beg, end, url))
    goto BAD_MESSAGE;

  iter = http::find_header(request_.headers, "Host");
  iter->value = url.host;
  request_.query = url.query;
  response_.message.clear();
  response_.headers.clear();
  redirect_count_++;
  connection_->close();
  start_op(url.host, determine_service(url), handler_);
  return;

  BAD_MESSAGE:
    http_err.assign(sys::errc::bad_message, sys::system_category());
    notify_error(http_err);
  return;

  OPERATION_CANCEL:
    http_err.assign(sys::errc::operation_canceled, sys::system_category()); 
    notify_error(http_err);
  return;
}

void agent::notify_header(boost::system::error_code const &err)
{
  handler_(err, request_, response_,
           asio::const_buffers_1(0,0));
}

void agent::notify_chunk(boost::system::error_code const &err)
{
  handler_(err, request_, response_, 
      connection_->io_buffer().data());
  connection_->io_buffer().consume(connection_->io_buffer().size());
}

void agent::notify_error(boost::system::error_code const &err)
{
  handler_(err, request_, response_, 
    connection_->io_buffer().data());
  connection_.reset();
}

