#ifndef AGENT_SESSION_HPP_
#define AGENT_SESSION_HPP_
#include <boost/asio/streambuf.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "agent/timeout_config.hpp"
#include "connection_handler_type.hpp"

class session_type
{
public:
  typedef boost::asio::streambuf      streambuf_type;
  typedef timeout_config              quality_config_type;
  typedef boost::asio::deadline_timer timer_type;
  
  session_type(boost::asio::io_service &io_service);
  ~session_type();

  streambuf_type          io_buffer;
  timer_type              timer;
  quality_config_type     quality_config;
  connect_handler_type    connect_handler;
  io_handler_type         io_handler;
  boost::shared_ptr<void>   extra_context;
};

#endif // header guard
