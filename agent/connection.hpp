#ifndef AGENT_CONNECTION_HPP_
#define AGENT_CONNECTION_HPP_
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>

class connection;
typedef boost::shared_ptr<connection> connection_ptr;

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

  connection(boost::asio::io_service &io_service);
  ~connection();
  
  void connect(
    std::string const &server, std::string const &port, 
    connect_handler_type handler);

  void read_some(boost::uint32_t at_least, io_handler_type handler);
  void read_until(char const* pattern, io_handler_type handler);
  void write(io_handler_type handler);

  // TODO write()
  streambuf_type &io_buffer();
  socket_type &socket();
  bool is_open() const;
  void close();
protected:
  void handle_resolve(
    const boost::system::error_code& err, 
    resolver::iterator endpoint,
    connect_handler_type handler);

  void handle_read(
    boost::system::error_code const& err, boost::uint32_t length, boost::uint32_t offset,
    io_handler_type handler);
private:
  boost::asio::io_service &io_service_;
  resolver resolver_;
  socket_type socket_;
  streambuf_type iobuf_;
};

#endif // header guard
