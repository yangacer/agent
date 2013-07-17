#ifndef AGENT_BASE_V2_HPP_
#define AGENT_BASE_V2_HPP_

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "agent/connection_fwd.hpp"
#include "agent/agent_handler_type.hpp"
#include "agent/entity.hpp"

class session_type;
class multipart;

class agent_base_v2
: private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
  typedef boost::shared_ptr<session_type> session_ptr;
  typedef boost::shared_ptr<agent_handler_type> handler_ptr;
  typedef boost::shared_ptr<multipart> multipart_ptr;
public:
  typedef agent_handler_type handler_type;
  typedef agent_monitor_type monitor_type;

  agent_base_v2(boost::asio::io_service &io_service);
  ~agent_base_v2();

  void async_request(
    http::entity::url const &url,
    http::request request, 
    std::string const &method,
    bool chunked_callback,
    handler_type handler,
    monitor_type monitor = monitor_type());

  boost::asio::io_service &io_service();
protected:

  struct context
  {
    context(boost::asio::io_service &ios, 
            http::request const &request,
            bool chunked_callback,
            handler_type const &handler,
            monitor_type const &monitor);
    ~context();

    http::request     request;
    http::response    response;
    unsigned char     redirect_count;
    bool              chunked_callback;
    boost::uintmax_t  expected_size;
    bool              is_redirecting;
    session_ptr       session;
    handler_type      handler;
    monitor_type      monitor;
    multipart_ptr     mpart;
  };
  typedef boost::shared_ptr<context> context_ptr;

  void handle_resolve(boost::system::error_code const &err, 
                      tcp::resolver::iterator endpoint,
                      context_ptr ctx_ptr);
  void handle_connect(boost::system::error_code const &err,
                      context_ptr ctx_ptr);
  void handle_write_request(boost::system::error_code const &err,
                            boost::uint32_t length,
                            context_ptr ctx_ptr);
  void handle_read_status_line(boost::system::error_code const &err,
                               context_ptr ctx_ptr);
  void handle_read_headers(boost::system::error_code const &err,
                           context_ptr ctx_ptr);
  void diagnose_transmission(context_ptr ctx_ptr);
  void read_chunk(context_ptr ctx_ptr);
  void handle_read_chunk(boost::system::error_code const &err, 
                         context_ptr ctx_ptr);
  void read_body(context_ptr ctx_ptr);
  void handle_read_body(boost::system::error_code const &err, 
                        boost::uint32_t length,
                        context_ptr ctx_ptr);
  void redirect(context_ptr ctx_ptr);
  void notify_chunk(boost::system::error_code const &err, 
                    boost::uint32_t length, 
                    context_ptr ctx_ptr);
  void notify_error(boost::system::error_code const &err, 
                    context_ptr ctx_ptr);
  void notify_monitor(context_ptr ctx_ptr,
                      agent_conn_action_t action, 
                      boost::uint32_t transfered);
  boost::asio::io_service   &io_service_;
private:
  connection_ptr            connection_;
  tcp::resolver             resolver_;
};


#endif
