#ifndef AGENT_HANDLER_TYPE_HPP_
#define AGENT_HANDLER_TYPE_HPP_

#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>
#include <boost/system/error_code.hpp>
#include "agent/entity.hpp"

typedef boost::function<
  void(
    boost::system::error_code const &,
    http::request const &,
    http::response const &, 
    boost::asio::const_buffer buffer)
  > agent_handler_type;

#endif
