#include "agent/agency.hpp"
#include <boost/ref.hpp>

// ---- fobidden handler ---

struct forbidden_handler
{
  void handle_request(boost::system::error_code const &err, 
                      http::request const & request,
                      agency &agency_, 
                      session_token_type token)
  {
    using http::entity::field;

    http::response resp =
      http::response::stock_response(http::status_type::forbidden);
    resp.headers << field("Content-Length", 0);
    agency_.async_reply_commit(
      request, resp, token, 
      boost::bind(&forbidden_handler::handle_commit, this, _1, _2, _3, _4));
  }

  void handle_commit(boost::system::error_code const &err, 
                     http::request const & request,
                     agency &agency_, 
                     session_token_type token)
  {}

};


agency::agency(std::string const &address, std::string const &port, 
               std::size_t thread_number)
: agency_base(address, port, thread_number)
{
  // setup forbidden handling
  boost::shared_ptr<forbidden_handler> fbh(new forbidden_handler);
  add_handler("_forbidden_", "GET", 
              boost::bind(&forbidden_handler::handle_request, fbh, 
                          _1, _2, _3, _4));
}

void agency::async_reply(http::request const &request, 
                         http::response const &response, 
                         session_token_type session_token,
                         handler_type handler)
{

}

void agency::async_reply_chunk(session_token_type session_token, 
                               boost::asio::const_buffer buffer, 
                               handler_type handler)
{

}

void agency::async_reply_commit(http::request const &request, 
                                session_token_type session_token, 
                                handler_type handler)
{

}

void agency::async_reply_commit(http::request const &request,
                                http::response const &response, 
                                session_token_type session_token,
                                handler_type handler)
{
  
}

void agency::notify(boost::system::error_code const &err,
                    http::request const &request,
                    boost::asio::io_service &io_service,
                    connection_ptr connection,
                    handler_type handler)
{
  io_service.post(
    boost::bind(handler,
                err, request, boost::ref(*this),
                boost::static_pointer_cast<void>(connection)));
}
