#include "connection.hpp"
#include <ostream>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/ref.hpp>
#include "agent/pre_compile_options.hpp"

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

// ---- conection impl ----
connection::connection(
  boost::asio::io_service &io_service, 
  timeout_config tconf)
  :  io_service_(io_service), resolver_(io_service), socket_(io_service),
     ctx_(boost::asio::ssl::context::tlsv1_client), sockets_(socket_, ctx_),
     is_secure_(false),ssl_short_read_error_(335544539),
     deadline_(io_service), timeout_config_(tconf)
{
  iobuf_.prepare(AGENT_CONNECTION_BUFFER_SIZE);
  sockets_.set_verify_mode(boost::asio::ssl::verify_none);
}

connection::~connection()
{}

void connection::connect(
  std::string const &server, std::string const &port, 
  connect_handler_type handler)
{
  is_secure_ = ("https" == port);
  resolver::query query(server, port);
  resolver_.async_resolve(
    query,
    boost::bind(
      &connection::handle_resolve, shared_from_this(), 
      asio_ph::error, asio_ph::iterator, handler));
}

#define CONNECTION_BIND_(Callback) \
  boost::bind(Callback, shared_from_this(), asio_ph::error, handler)

#define SET_TIMER_(Seconds, Callback) \
  deadline_.expires_from_now( \
    boost::posix_time::seconds(Seconds)); \
  deadline_.async_wait( CONNECTION_BIND_(Callback) );
    //boost::bind( \
    //Callback, shared_from_this(), asio_ph::error, handler));

void connection::handle_resolve(
  const boost::system::error_code& err, 
  resolver::iterator endpoint,
  connect_handler_type handler)
{
  if (!err && endpoint != tcp::resolver::iterator()) {
    if(is_secure_){
      asio::async_connect(socket_, endpoint, 
        CONNECTION_BIND_(&connection::connect_secure));
    }else {
      asio::async_connect(socket_, endpoint, 
        CONNECTION_BIND_(&connection::handle_connect));
    }
    SET_TIMER_(timeout_config_.connect(), 
               &connection::handle_connect_timeout);
  } else {
    io_service_.post(boost::bind(handler,err));
  }
}

void connection::connect_secure(
  const boost::system::error_code& err,
  connect_handler_type handler)
{
  if (!err) {
    sockets_.async_handshake(
      asio::ssl::stream_base::client, 
      CONNECTION_BIND_(&connection::handle_connect));
    
    SET_TIMER_(timeout_config_.connect(), 
               &connection::handle_connect_timeout);
  } else {
    io_service_.post(boost::bind(handler,err));
  }
}

void connection::handle_connect(
  const boost::system::error_code& err, 
  connect_handler_type handler)
{
  deadline_.cancel();
  io_service_.post(boost::bind(handler, err));
}

void connection::read_some(boost::uint32_t at_least, io_handler_type handler)
{
  if(is_secure_ == true) {
    boost::asio::async_read(
      sockets_, iobuf_,
      boost::asio::transfer_at_least(at_least),
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, iobuf_.size(), handler));
  } else {
    boost::asio::async_read(
      socket_, iobuf_,
      boost::asio::transfer_at_least(at_least),
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, iobuf_.size(), handler));
  }
}

void connection::read_until(char const* pattern, io_handler_type handler)
{
  if(is_secure_ == true) {
    boost::asio::async_read_until(
      sockets_, iobuf_, pattern,
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, 0, handler));
  } else {
    boost::asio::async_read_until(
      socket_, iobuf_, pattern,
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, 0, handler));
  }
}

void connection::handle_read(
    boost::system::error_code const& err, boost::uint32_t length, 
    boost::uint32_t offset, io_handler_type handler)
{
  boost::system::error_code err_ = err;
  bool is_ssl_short_read_error_ = 
    (err.category() == boost::asio::error::ssl_category &&
    err.value() == ssl_short_read_error_);
  if(err && is_ssl_short_read_error_)
    err_ = boost::asio::error::eof;
  io_service_.post(
    boost::bind(handler, err_, length));
  // TODO add speed limitation
}

void connection::write(io_handler_type handler)
{
  if(is_secure_ == true){
    boost::asio::async_write(
      sockets_, iobuf_, boost::bind(handler, _1, _2));
  } else {
    boost::asio::async_write(
      socket_, iobuf_, boost::bind(handler, _1, _2));
  }
}

connection::streambuf_type &
connection::io_buffer() { return iobuf_; }

connection::socket_type &
connection::socket() {  return socket_; }

timeout_config &
connection::timeout() { return timeout_config_; }

bool connection::is_open() const
{  return socket_.is_open(); }

void connection::close() {  socket_.close(); }

void connection::handle_connect_timeout(
  boost::system::error_code const &err,
  connect_handler_type handler)
{
  if(!err) {
    sys::error_code ec(sys::errc::timed_out, sys::system_category());
    io_service_.post(boost::bind(handler, ec));
  } 
}

