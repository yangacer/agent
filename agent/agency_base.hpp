#ifndef AGENT_AGENCY_BASE_HPP_
#define AGENT_AGENCY_BASE_HPP_

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include "agent/io_service_pool.hpp"
#include "agent/connection_fwd.hpp"
#include "agent/agency_handler_type.hpp"

class session_type;

class agency_base
{
protected:
  typedef boost::shared_ptr<session_type> session_ptr;
  typedef boost::asio::ip::tcp tcp;
public:
  typedef agency_handler_type handler_type;
  
  agency_base(std::string const &address, std::string const &port, 
              std::size_t thread_number);
  ~agency_base(); 

  void set_handler(std::string const &uri_prefix, std::string const &method, 
                   handler_type handler);

  void add_handler(std::string const &uri_prefix, std::string const &method, 
                   handler_type handler);

  void del_handler(std::string const &uri_prefix, std::string const &method);
  
  agency_handler_type get_handler(std::string const &uri, 
                                  std::string const &method);
  void run();

  void async_reply(http::request const &request, 
                   http::response const &response, 
                   session_ptr session,
                   handler_type handler);

  void async_reply_chunk(http::request const &request,
                         session_ptr session,
                         boost::asio::const_buffer buffer, 
                         handler_type handler);
  
  void async_reply_commit(http::request const &request, 
                          session_ptr session,
                          handler_type handler);
  
  void async_reply_commit(http::request const &request,
                          http::response const &response, 
                          session_ptr session,
                          handler_type handler);

protected:
  void start_accept();
  void handle_accept(boost::system::error_code const &err);
  void start_read(session_ptr session);
  void handle_read_request_line(
    boost::system::error_code const &err, 
    boost::uint32_t length,
    session_ptr session);

  void handle_read_header_list(
    boost::system::error_code const &err, 
    boost::uint32_t length,
    session_ptr session,
    http::request request);

  void handle_read_body(
    boost::system::error_code const &err, 
    boost::uint32_t length,
    session_ptr session,
    http::request const &request);

  void handle_reply(boost::system::error_code const &err,
                    http::request const &request,
                    session_ptr session,
                    handler_type handler);

  void handle_reply_commit(boost::system::error_code const &err, 
                           http::request const &request,
                           session_ptr session,
                           handler_type handler);

  void handle_stop();
  
  virtual void notify(boost::system::error_code const &err,
                      http::request const &request,
                      boost::asio::io_service &io_service,
                      session_ptr session,
                      boost::asio::const_buffer buffer,
                      handler_type handler) = 0;
private:
  io_service_pool io_service_pool_;
  boost::asio::signal_set signal_set_;
  boost::asio::ip::tcp::acceptor acceptor_;
  connection_ptr connection_;
  std::map<
    std::string, 
    std::map<std::string, handler_type> 
      > handlers_;
};

#endif
