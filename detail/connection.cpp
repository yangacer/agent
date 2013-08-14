#include "connection.hpp"
#include <ostream>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/ref.hpp>
#include "agent/pre_compile_options.hpp"
#include "session.hpp"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#include "agent/log.hpp"
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, this);
#else
#   define AGENT_TRACKING(Desc)
#endif

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

namespace detail {
  bool is_secured(std::string const &port)
  { 
    return port == "https" || port == "443" || 
      port == "445"; 
  }
}

// ---- conection impl ----
connection::connection(
  boost::asio::io_service &io_service)
  : io_service_(io_service), 
    deadline_timer_(io_service),
    resolver_(io_service), 
    socket_(io_service),
    ssl_short_read_error_(335544539)
{
  AGENT_TRACKING("connection::ctor");
}

connection::~connection()
{
  AGENT_TRACKING("connection::dtor");
}

boost::asio::io_service &connection::get_io_service()
{ return io_service_; }

// GENERIC_BIND_: bind error_code and handler to be called 
#define GENERIC_BIND_(Callback) \
  boost::bind(Callback, shared_from_this(), \
              asio_ph::error,\
              boost::ref(session))

#define SET_TIMER_ \
{  deadline_timer_.expires_from_now(boost::posix_time::seconds(20)); }

void connection::connect(resolver::iterator endpoint, 
                         session_type &session)
{
  AGENT_TRACKING("connection::connect");

  bool is_ssl = 
    endpoint->endpoint().port() == 443 ||
    endpoint->endpoint().port() == 445;

  if(is_ssl){
    // initialize ssl_socket_
    ssl_socket_.reset(new ssl_socket(
        boost::asio::ssl::context::tlsv1_client,
        socket_));
    ssl_socket_->socket.set_verify_mode(boost::asio::ssl::verify_none);

    asio::async_connect(socket_, endpoint, 
                        GENERIC_BIND_(&connection::handle_secured_connect));
  }else {
    asio::async_connect(socket_, endpoint, 
                        GENERIC_BIND_(&connection::handle_connect));
  }
  check_deadline();
  SET_TIMER_;
}

void connection::handle_secured_connect(
  const boost::system::error_code& err,
  session_type &session)
{
  AGENT_TRACKING("connection::handle_secured_connect");

  if (!err) {
    ssl_socket_->socket.async_handshake(
      asio::ssl::stream_base::client, 
      GENERIC_BIND_(&connection::handle_connect));
    
    SET_TIMER_;
  } else {
    io_service_.post(boost::bind(session.connect_handler, err));
    session.connect_handler = connect_handler_type();
  }
}

void connection::handle_connect(
  const boost::system::error_code& err, 
  session_type &session)
{
  AGENT_TRACKING("connection::handle_connect");
  io_service_.post(boost::bind(session.connect_handler, err));
  session.connect_handler = connect_handler_type();
}

// IO_BIND_: Bind error_code, length, and offset to be called
#define IO_BIND_(Callback, Offset) \
  boost::bind(Callback, shared_from_this(), _1, _2, \
              Offset, boost::ref(session))

void connection::read_some(boost::uint32_t at_least, session_type &session)
{
  AGENT_TRACKING("connection::read_some");
  if(ssl_socket_) {
    boost::asio::async_read(
      ssl_socket_->socket, session.io_buffer,
      boost::asio::transfer_at_least(at_least),
      IO_BIND_(&connection::handle_read, session.io_buffer.size()));
  } else {
    boost::asio::async_read(
      socket_, session.io_buffer,
      boost::asio::transfer_at_least(at_least),
      IO_BIND_(&connection::handle_read, session.io_buffer.size()));
  }
  
  SET_TIMER_;
}

void connection::read_until(char const* pattern, boost::uint32_t at_most, session_type &session)
{
  AGENT_TRACKING("connection::read_until");
  if(ssl_socket_) {
    boost::asio::async_read_until(
      ssl_socket_->socket, session.io_buffer, pattern,
      IO_BIND_(&connection::handle_read, 0));
  } else {
    boost::asio::async_read_until(
      socket_, session.io_buffer, pattern,
      IO_BIND_(&connection::handle_read, 0));
  }
  SET_TIMER_;
}

void connection::handle_read(
    boost::system::error_code const& err, boost::uint32_t length, 
    boost::uint32_t offset, 
    session_type &session)
{
  AGENT_TRACKING("connection::handle_read");
  boost::system::error_code err_ = err;
  bool is_ssl_short_read_error_ = 
    (err.category() == boost::asio::error::ssl_category &&
    err.value() == ssl_short_read_error_);

  if(err && is_ssl_short_read_error_)
    err_ = boost::asio::error::eof;
  io_service_.post(
    boost::bind(session.io_handler, err_, length));
  session.io_handler = io_handler_type();
}

void connection::write(session_type &session)
{
  AGENT_TRACKING("connection::write");
  if(ssl_socket_){
    boost::asio::async_write(
      ssl_socket_->socket, session.io_buffer, 
      IO_BIND_(&connection::handle_write, 0));
  } else {
    boost::asio::async_write(
      socket_, session.io_buffer, 
      IO_BIND_(&connection::handle_write, 0));
  }
  SET_TIMER_;
}

void connection::write(session_type &session, boost::asio::const_buffer buffer)
{
  AGENT_TRACKING("connection::write");
  auto buf = asio::buffer(buffer);
  if(ssl_socket_){
    boost::asio::async_write(
      ssl_socket_->socket, buf, 
      IO_BIND_(&connection::handle_write, 0));
  } else {
    boost::asio::async_write(
      socket_, buf, 
      IO_BIND_(&connection::handle_write, 0));
  }
  SET_TIMER_;
}

void connection::handle_write(
  boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    session_type &session)
{
  AGENT_TRACKING("connection::handle_write");
  io_service_.post(
    boost::bind(session.io_handler, err, length));
  session.io_handler = io_handler_type();
}

connection::socket_type &
connection::socket() {  return socket_; }

bool connection::is_open() const
{  return socket_.is_open(); }

void connection::close() 
{ 
  sys::error_code ec;
  deadline_timer_.cancel();
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
}

void connection::check_deadline()
{
  AGENT_TRACKING("connection::check_deadline");
  if( deadline_timer_.expires_at() < timer_type::traits_type::now()) {
    AGENT_TRACKING("connection::check_deadline(timeout)");
    close();
  } else {
    deadline_timer_.async_wait(boost::bind(&connection::check_deadline, shared_from_this()));
  }
}
