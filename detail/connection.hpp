#ifndef AGENT_CONNECTION_HPP_
#define AGENT_CONNECTION_HPP_
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "agent/connection_fwd.hpp"
#include "agent/timeout_config.hpp"

class connection 
: public boost::enable_shared_from_this<connection>,
  private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
public:
  typedef boost::function<
    void(boost::system::error_code const&)
    > connect_handler_type;

  typedef boost::function<
    void(boost::system::error_code const&, boost::uint32_t)
    > io_handler_type;
  typedef boost::asio::streambuf streambuf_type;
  typedef boost::asio::ip::tcp::socket socket_type;

  connection(boost::asio::io_service &io_service, 
             timeout_config tconf = timeout_config());
  ~connection();
  
  void connect(
    std::string const &server, std::string const &port, 
    connect_handler_type handler);

  void read_some(boost::uint32_t at_least, io_handler_type handler);
  void read_until(char const* pattern, boost::uint32_t at_most, io_handler_type handler);
  void write(io_handler_type handler);

  streambuf_type &io_buffer();
  socket_type &socket();
  timeout_config &timeout();
  bool is_open() const;
  void close();
protected:
  void connect_secure(
    const boost::system::error_code& err,
    connect_handler_type handler);

  void handle_resolve(
    const boost::system::error_code& err, 
    resolver::iterator endpoint,
    connect_handler_type handler);
  
  void handle_connect(
    const boost::system::error_code& err, 
    connect_handler_type handler);

  void handle_read(
    boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    io_handler_type handler);

  void handle_write(
    boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    io_handler_type handler);

  void handle_connect_timeout(
    boost::system::error_code const &err,
    connect_handler_type handler);

  void handle_io_timeout(
    boost::system::error_code const &err,
    io_handler_type handler);

private:
  boost::asio::io_service &io_service_;
  resolver    resolver_;
  socket_type socket_;
  boost::asio::ssl::context ctx_;
  boost::asio::ssl::stream<socket_type&> sockets_;
  streambuf_type iobuf_;
  bool is_secure_;
  const long ssl_short_read_error_;
  boost::asio::deadline_timer deadline_;
  timeout_config timeout_config_;
};

#endif // header guard
