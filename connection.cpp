#include "connection.hpp"
#include <ostream>
#include <boost/asio.hpp>
#include <boost/ref.hpp>

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

connection::connection(boost::asio::io_service &io_service)
  :  io_service_(io_service), resolver_(io_service), socket_(io_service)
{}

connection::~connection()
{}

void connection::connect(
  std::string const &server, std::string const &port, 
  connect_handler_type handler)
{
  resolver::query query(server, port);
  // XXX is this reliable?
  iobuf_.consume(iobuf_.size());
  resolver_.async_resolve(
    query,
    boost::bind(
      &connection::handle_resolve, shared_from_this(), 
      asio_ph::error, asio_ph::iterator, handler));
}

void connection::read_some(io_handler_type handler) // TODO timeout
{
  if( iobuf_.size() ) {
    io_service_.post(
      boost::bind(handler, sys::error_code(), 
                  iobuf_.size(), shared_from_this()));
    iobuf_.consume(iobuf_.size());
  } else {
    boost::asio::async_read(
      socket_,
      iobuf_,
      boost::asio::transfer_at_least(1),
      boost::bind(
        &connection::handle_read, shared_from_this(), 
        _1, _2, handler));
  }
}

connection::streambuf_type &
connection::io_buffer()
{ return iobuf_; }

connection::socket_type &
connection::socket()
{ return socket_; }

bool connection::is_open() const
{ return socket_.is_open(); }

void connection::close()
{  socket_.close();  }

void connection::handle_resolve(
  const boost::system::error_code& err, 
  resolver::iterator endpoint,
  connect_handler_type handler)
{
  if (!err && endpoint != tcp::resolver::iterator()) {
    asio::async_connect(socket_, endpoint, boost::bind(handler, asio_ph::error));
  } else {
    io_service_.post(boost::bind(handler,err));
  }
}

void connection::handle_read(
    boost::system::error_code const& err, boost::uint32_t length, 
    io_handler_type handler)
{
  io_service_.post(
    boost::bind(handler, err, length, shared_from_this()));
  iobuf_.consume(iobuf_.size());
}

