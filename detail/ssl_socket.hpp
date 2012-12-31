#ifndef AGENT_SSL_STREAM_HPP_
#define AGENT_SSL_STREAM_HPP_

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>

struct ssl_socket
{
  typedef boost::asio::ip::tcp::socket socket_type;

  ssl_socket(
    boost::asio::ssl::context::method method,
    socket_type &underlying_socket)
    : context(method), 
      socket(underlying_socket, context)
    {}

  boost::asio::ssl::context context;
  boost::asio::ssl::stream<socket_type&> socket;
};

typedef boost::shared_ptr<ssl_socket> ssl_socket_ptr;

#endif
