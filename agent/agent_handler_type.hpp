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

enum agent_conn_action_t
{
  upstream = 0,
  downstream
};

typedef boost::function<
  void(agent_conn_action_t, // connection action
       boost::uintmax_t,    // total transfered (0 if unknown)
       boost::uint32_t)     // bytes transfered
  > agent_monitor_type;

// TODO add agent& to interface
#endif
