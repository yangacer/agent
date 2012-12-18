#include "agent/connection.hpp"
#include <ostream>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/connect.hpp>
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
  iobuf_.consume(iobuf_.size());
  resolver_.async_resolve(
    query,
    boost::bind(
      &connection::handle_resolve, shared_from_this(), 
      asio_ph::error, asio_ph::iterator, handler));
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
    boost::uint32_t offset, io_handler_type handler)
{
  io_service_.post(
    boost::bind(handler, err, length + offset, shared_from_this()));
  // TODO add speed limitation
}

