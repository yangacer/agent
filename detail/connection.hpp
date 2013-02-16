#ifndef AGENT_CONNECTION_HPP_
#define AGENT_CONNECTION_HPP_
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl.hpp>
#include "agent/connection_fwd.hpp"
#include "agent/timeout_config.hpp"
#include "ssl_socket.hpp"
#include "connection_handler_type.hpp"

class session_type;

class connection 
: public boost::enable_shared_from_this<connection>,
  private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
public:
  typedef boost::asio::ip::tcp::socket socket_type;

  connection(boost::asio::io_service &io_service);
  ~connection();
  
  void connect(resolver::iterator endpoint, 
               session_type &session);

  void read_some(boost::uint32_t at_least, session_type &session);
  void read_until(char const* pattern, boost::uint32_t at_most, session_type &session);
  void write(session_type &session);

  boost::asio::io_service &get_io_service();
  socket_type &socket();
  bool is_open() const;
  void close();
protected:
  void handle_secured_connect(
    const boost::system::error_code& err,
    session_type &session);

  void handle_connect(
    const boost::system::error_code& err, 
    session_type &session);

  void handle_read(
    boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    session_type &session);

  void handle_write(
    boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    session_type &session);

  void handle_connect_timeout(
    boost::system::error_code const &err,
    session_type &session);

  void handle_io_timeout(
    boost::system::error_code const &err,
    session_type &session);

private:
  boost::asio::io_service   &io_service_;
  resolver                  resolver_;
  socket_type               socket_;
  ssl_socket_ptr ssl_socket_;
  const long  ssl_short_read_error_;
};
// TODO Replace IO timeout. Do not report slow IO active. 

#endif // header guard
