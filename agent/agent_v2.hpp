#ifndef AGENT_V2_HPP_
#define AGENT_V2_HPP_

#include "agent/agent_base_v2.hpp"

class agent_v2
: public agent_base_v2
{
public:
  agent_v2(boost::asio::io_service &io_service);

  void async_request(
    http::entity::url const &url,
    http::request request, 
    std::string const &method,
    bool chunked_callback,
    handler_type handler);
};


#endif
