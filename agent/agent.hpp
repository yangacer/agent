#ifndef AGENT_HPP_
#define AGENT_HPP_

#include "agent/agent_base.hpp"

class agent
: public agent_base
{
public:
  agent(boost::asio::io_service &io_service);
  ~agent();

  void async_get(http::entity::url const &url, bool chunked_callback,
                 handler_type handler);

  void async_get(http::entity::url const &url, bool chunked_callback,
                 boost::uint64_t offset, boost::uint64_t size,
                 handler_type handler);

  void async_post(http::entity::url const &url, 
                  http::entity::query_map_t const &post_parameter,
                  bool chunked_callback,
                  handler_type handler);

  void async_abort();
};

#endif
