#include "agent/agent.hpp"
#include <ostream>
#include <string>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "agent/parser.hpp"
#define AGENT_MAXIMUM_REDIRECT_COUNT 5

std::string 
determine_service(http::entity::url const & url)
{
  return (url.port) ?
    boost::lexical_cast<std::string>(url.port) :
    url.scheme
    ;
}

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

agent::agent(asio::io_service &ios)
: io_service_(ios), redirect_count_(0)  
{}

agent::~agent()
{}

void agent::operator()(std::string const &server, std::string const &port, 
                       handler_type handler)
{ 
  redirect_count_ = 0;
  start_op(server, port, handler); 
}

void agent::operator()(std::string const &url, handler_type handler)
{ fetch(url, handler); }

void agent::fetch(std::string const &url, handler_type handler)
{
  sys::error_code http_err;
  http::entity::url url_;
  auto beg(url.begin()), end(url.end());

  if(!http::parser::parse_url(beg, end, url_)) {
    http_err.assign(sys::errc::invalid_argument, sys::system_category());
    notify_error(http_err);
    return;
  }
  request_.headers << http::entity::field("Host", url_.host);
  request_.query = url_.query;
  this->operator()(url_.host, determine_service(url_), handler);
}

http::request &agent::request()
{  return request_;  }

void agent::start_op(
  std::string const &server, std::string const &port, handler_type handler)
{
  // TODO global management of connection
  connection_.reset(new connection(io_service_));
  handler_ = handler;
  connection_->connect(
    server, port,
    boost::bind(
      &agent::handle_connect, this, asio_ph::error));
}

void agent::handle_connect(boost::system::error_code const &err)
{
  if(!err) {
    std::ostream in(&connection_->io_buffer());
    in.flush();
    in << request_;
    asio::async_write(
      connection_->socket(), connection_->io_buffer(),
      boost::bind(
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
  if (!err){
    connection_->io_buffer().consume(len);
    asio::async_read_until(
      connection_->socket(), connection_->io_buffer(), "\r\n",
      boost::bind(&agent::handle_read_status_line, this,
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
      connection_.reset();
      io_service_.post(
        boost::bind(handler_, http_err, request_,
                    response_, connection_));
      return;
    }
    connection_->io_buffer().consume(
      beg - asio::buffers_begin(connection_->io_buffer().data()));
    // Read the response headers, which are terminated by a blank line.
    asio::async_read_until(
      connection_->socket(), 
      connection_->io_buffer(), 
      "\r\n\r\n",
      boost::bind(
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
      connection_.reset();
      io_service_.post(
        boost::bind(handler_, http_err, request_,
                    response_, connection_));
      return;
    }

    connection_->io_buffer().consume(
      beg - asio::buffers_begin(connection_->io_buffer().data()));
    // TODO better log
    // Handle redirection - i.e. 301, 302, 
    if(response_.status_code >= 300 && response_.status_code < 400){
      redirect();
    } else {
      io_service_.post(
        boost::bind(handler_, err, request_, response_, connection_));
    }
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

void agent::notify_error(boost::system::error_code const &err)
{
  connection_.reset();
  io_service_.post(
    boost::bind(handler_, err, request_, response_, connection_));
}

