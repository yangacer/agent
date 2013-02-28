#include "agent/agency.hpp"
#include <boost/ref.hpp>
#include <boost/asio/buffer.hpp>
#include "agent/log.hpp"
#include "connection.hpp"
#include "session.hpp"
#include "agent/log.hpp"
#include "agent/pre_compile_options.hpp"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#else
#   define AGENT_TRACKING(Desc)
#endif
// ---- fobidden handler ---

struct forbidden_handler
{
  void handle_request(boost::system::error_code const &err, 
                      http::request const & request,
                      agency &agency_, 
                      session_ptr session,
                      boost::asio::const_buffer)
  {
    AGENT_TRACKING("forbidden_handler::handle_request");
    using http::entity::field;
    if( err == boost::asio::error::eof ) {
      http::response resp =
        http::response::stock_response(http::status_type::forbidden);
      resp.headers << field("Content-Length", 0);
      agency_.async_reply_commit(
        request, resp, session, 
        boost::bind(&forbidden_handler::handle_commit, this, _1));//, _2, _3, _4, _5));
    }
  }

  void handle_commit(boost::system::error_code const &err)//, 
  {
    AGENT_TRACKING("forbidden_handler::handle_commit");
  }

};

// --- agency impl ----

agency::agency(std::string const &address, std::string const &port, 
               std::size_t thread_number)
: agency_base(address, port, thread_number)
{
  // setup forbidden handling
  boost::shared_ptr<forbidden_handler> fbh(new forbidden_handler);
  add_handler("_forbidden_", "GET", 
              boost::bind(&forbidden_handler::handle_request, fbh, 
                          _1, _2, _3, _4, _5));
}

void agency::async_reply(http::request const &request, 
                         http::response const &response, 
                         session_ptr session,
                         handler_type handler)
{
  typedef void(agency_base::*mem_fn_type)(
    http::request const&, http::response const &,
    session_ptr, handler_type);

}

void agency::async_reply_chunk(session_ptr session, 
                               boost::asio::const_buffer buffer, 
                               handler_type handler)
{
  typedef void(agency_base::*mem_fn_type)(
    session_ptr, boost::asio::const_buffer buffer, handler_type);
}

void agency::async_reply_commit(http::request const &request, 
                                session_ptr session,
                                handler_type handler)
{
  typedef void(agency_base::*mem_fn_type)(
    http::request const&, session_ptr, handler_type);
  
}

void agency::async_reply_commit(http::request const &request,
                                http::response const &response, 
                                session_ptr session,
                                handler_type handler)
{
  typedef void(agency_base::*mem_fn_type)(
    http::request const&, http::response const &,
    session_ptr, handler_type);

  AGENT_TRACKING("agency::async_reply_commit");
  session->connection->get_io_service().post(boost::bind(
      (mem_fn_type)&agency_base::async_reply_commit, this, 
      request, response, session, handler));
}

void agency::notify(boost::system::error_code const &err,
                    http::request const &request,
                    boost::asio::io_service &io_service,
                    session_ptr session,
                    boost::asio::const_buffer buffer,
                    handler_type handler)
{
  io_service.post(
    boost::bind(
      handler, err, request, 
      boost::ref(*this), session, buffer));
}
