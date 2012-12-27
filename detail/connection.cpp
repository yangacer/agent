#include "connection.hpp"
#include <ostream>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/ref.hpp>

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

connection::connection(boost::asio::io_service &io_service)
  :  io_service_(io_service), resolver_(io_service), socket_(io_service),
     ctx_(boost::asio::ssl::context::tlsv1_client), sockets_(socket_, ctx_),
     is_secure_(false),short_read_error(335544539)
{
  iobuf_.prepare(2048);
  sockets_.set_verify_mode(boost::asio::ssl::verify_none);
}

connection::~connection()
{}

void connection::connect(
  std::string const &server, std::string const &port, 
  connect_handler_type handler)
{
  if(port == "https"){
    is_secure_ = true;
  }
  resolver::query query(server, port);
  resolver_.async_resolve(
    query,
    boost::bind(
      &connection::handle_resolve, shared_from_this(), 
      asio_ph::error, asio_ph::iterator, handler));
}

void connection::read_some(boost::uint32_t at_least, io_handler_type handler)
{
  if(is_secure_ == true){
    boost::asio::async_read(
      sockets_,
      iobuf_,
      boost::asio::transfer_at_least(at_least),
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, iobuf_.size(), handler));
    return;
  }
  boost::asio::async_read(
    socket_,
    iobuf_,
    boost::asio::transfer_at_least(at_least),
    boost::bind(
      &connection::handle_read, shared_from_this(), 
      _1, _2, iobuf_.size(), handler));
}

void connection::read_until(char const* pattern, io_handler_type handler)
{
  if(is_secure_ == true){
    boost::asio::async_read_until(
      sockets_,
      iobuf_,
      pattern,
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, 0, handler));
    return;
  }
  boost::asio::async_read_until(
    socket_,
    iobuf_,
    pattern,
    boost::bind(
      &connection::handle_read, shared_from_this(), 
      _1, _2, 0, handler));
}

void connection::handle_read(
    boost::system::error_code const& err, boost::uint32_t length, 
    boost::uint32_t offset, io_handler_type handler)
{
  boost::system::error_code err_ = err;
  bool is_short_read_error = 
    (err.category() == boost::asio::error::ssl_category &&
    err.value() == short_read_error);
  if(err && is_short_read_error)
    err_ = boost::asio::error::eof;
  io_service_.post(
    boost::bind(handler, err_, length));
  // TODO add speed limitation
}

void connection::write(io_handler_type handler)
{
  if(is_secure_ == true){
    boost::asio::async_write(
      sockets_, 
      iobuf_, 
      boost::bind(handler, _1, _2));
    return;
  }
  boost::asio::async_write(
    socket_, 
    iobuf_, 
    boost::bind(handler, _1, _2));
}

connection::streambuf_type &
connection::io_buffer()
{ return iobuf_; }

connection::socket_type &
connection::socket()
{
  return socket_;
}

bool connection::is_open() const
{
  return socket_.is_open();
}

void connection::close()
{
  socket_.close();
}

void connection::handle_resolve(
  const boost::system::error_code& err, 
  resolver::iterator endpoint,
  connect_handler_type handler)
{
  if (!err && endpoint != tcp::resolver::iterator()) {
    if(is_secure_){
      asio::async_connect(socket_, endpoint, boost::bind(&connection::connect_secure, shared_from_this(), asio_ph::error, handler));
      return;
    }
    asio::async_connect(socket_, endpoint, boost::bind(handler, asio_ph::error));
  } else {
    io_service_.post(boost::bind(handler,err));
  }
}

void connection::connect_secure(
  const boost::system::error_code& err,
  connect_handler_type handler)
{
  if (!err) {
    sockets_.async_handshake(asio::ssl::stream_base::client, boost::bind(&connection::handle_handshake, shared_from_this(), asio_ph::error, handler));
  } else {
    io_service_.post(boost::bind(handler,err));
  }
}

void connection::handle_handshake(
  const boost::system::error_code& err,
  connect_handler_type handler)
{
    io_service_.post(boost::bind(handler,err));
}

