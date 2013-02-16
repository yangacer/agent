#ifndef AGENT_AGENCY_HPP_
#define AGENT_AGENCY_HPP_

#include "agent/agency_base.hpp"

class agency
: public agency_base
{
public:
  agency(std::string const &address, std::string const &port, 
         std::size_t thread_number);
  
  void async_reply(http::request const &request, 
                   http::response const &response, 
                   session_token_type session_token,
                   handler_type handler);

  void async_reply_chunk(session_token_type session_token, 
                         boost::asio::const_buffer buffer, 
                         handler_type handler);
  
  void async_reply_commit(http::request const &request, 
                          session_token_type session_token, 
                          handler_type handler);
  
  void async_reply_commit(http::request const &request,
                          http::response const &response, 
                          session_token_type session_token,
                          handler_type handler);
protected:
  virtual void notify(boost::system::error_code const &err,
                      http::request const &request,
                      boost::asio::io_service &io_service,
                      connection_ptr connection,
                      handler_type handler);
};

#endif
