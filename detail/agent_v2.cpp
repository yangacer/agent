#include "agent/agent_v2.hpp"
#include <boost/bind.hpp>
#include <boost/ref.hpp>

agent_v2::agent_v2(boost::asio::io_service &io_service)
  : agent_base_v2(io_service)
{}

void agent_v2::async_request(
  http::entity::url const &url,
  http::request request, 
  std::string const &method,
  bool chunked_callback,
  handler_type handler,
  monitor_type monitor)
{
  io_service_.post(boost::bind(
      &agent_base_v2::async_request, this,
      url, request, 
      method,
      chunked_callback,
      handler,
      monitor));
}

boost::asio::io_service &agent_v2::io_service()
{
  return agent_base_v2::io_service();
}
