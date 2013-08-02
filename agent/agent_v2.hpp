#ifndef AGENT_V2_HPP_
#define AGENT_V2_HPP_

#include "agent/agent_base_v2.hpp"

class agent_v2
: public agent_base_v2
{
public:
  agent_v2(boost::asio::io_service &io_service);

  /** Send HTTP request asynchronously.
   * @param url A http url that can be initiated with literal text. e.g.
   * @codebeg
   * // POST request 
   * using http::entity::field;
   * // Note the literal data must be escaped. Otherwise you need to use 
   * // url.query.query_map for inserting unencoded data.
   * http::entity::url url("http://localhost/post?param=value&%40file=/usr/myfile");
   * http::request request;
   * request.headers << field("User-Agent", "My agent");
   *
   * agent.async_request(url, request, "POST", true, boost::bind(&callback, _1, _2, _3, _4));
   *
   * @endcode
   * @param request User supplied request may contain extra headers.
   * @param method "GET" or "POST".
   * @param chunked_callback Determine whether to notify client once a chunk
   * of response is received.
   * @param handler Client's handler.
   */
  void async_request(
    http::entity::url const &url,
    http::request request, 
    std::string const &method,
    bool chunked_callback,
    handler_type handler,
    monitor_type monitor = monitor_type());

  boost::asio::io_service& io_service();
};


#endif
