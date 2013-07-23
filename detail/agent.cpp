#include "agent/agent.hpp"
#include <boost/bind.hpp>

agent::agent(boost::asio::io_service &io_service)
: agent_base(io_service)
{}

agent::~agent()
{}


void agent::async_get(http::entity::url const& url, bool chunked_callback,
                           handler_type handler)
{
  typedef void(agent_base::*mem_fn_type)(
    http::entity::url const&, bool, handler_type);
  io_service().post(
    boost::bind((mem_fn_type)&agent_base::async_get, this, url, chunked_callback,
                handler));
}

void agent::async_get(http::entity::url const &url, bool chunked_callback,
               boost::uintmax_t offset, boost::uintmax_t size,
               handler_type handler)
{
  typedef void(agent_base::*mem_fn_type)(
    http::entity::url const&, bool, boost::uintmax_t, boost::uintmax_t, handler_type);
  io_service().post(
    boost::bind((mem_fn_type)&agent_base::async_get, this, url, chunked_callback,
                offset, size, handler));
  
}

void agent::async_post(http::entity::url const &url, 
                            http::entity::query_map_t const &post_parameter,
                            bool chunked_callback,
                            handler_type handler)
{
  io_service().post(
    boost::bind(&agent_base::async_post, this, url, post_parameter,
                chunked_callback, handler));
}

void agent::async_abort()
{
  io_service().post(
    boost::bind(&agent_base::async_abort, this));
}
