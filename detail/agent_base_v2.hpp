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

class agent_base_v2
: private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
  typedef boost::shared_ptr<session_type> session_ptr;
  typedef boost::shared_ptr<agent_handler_type> handler_ptr;
public:
  typedef agent_handler_type handler_type;
  
  agent_base_v2(boost::asio::io_service &io_service);
  ~agent_base_v2();

  void async_get(
    http::entity::url const &url,
    http::request const &request, 
    bool chunked_callback,
    handler_type handler);

  /*
  void async_post(http::request const &request,
                  bool chunked_callback,
                  handler_type handler);
  */
  void async_post_multipart(
    http::request const &request,
    std::vector<std::string> files,
    handler_type handler);
  /*
  void async_abort();
  boost::asio::io_service& io_service();
  */
protected:

  struct context
  {
    ~context();
    http::request request;
    http::response response;
    unsigned char redirect_count;
    bool chunked_calback;
    boost::uintmax_t expected_size;
    bool is_redirecting;
    connection_ptr  connection;
    session_ptr   session;
    handler_type  handler;
    //multipart     multipart_ctx;
  };
  typedef boost::shared_ptr<context> context_ptr;

  void handle_resolve(boost::system::error_code const &err, 
                      tcp::resolver::iterator endpoint,
                      context_ptr ctx_ptr);
  void handle_connect(boost::system::error_code const &err,
                      context_ptr ctx_ptr);
  /*
  void write_request();
  void handle_write_request(boost::system::error_code const &err,
                            boost::uint32_t len);
  void handle_read_status_line(boost::system::error_code const &err);
  void handle_read_headers(boost::system::error_code const &err);
  void redirect();
  void diagnose_transmission();
  void read_chunk();
  void handle_read_chunk(boost::system::error_code const &err);
  void read_body();
  void handle_read_body(boost::system::error_code const &err, boost::uint32_t length);
  */
private:
  boost::asio::io_service &io_service_;
  tcp::resolver   resolver_;
};


#endif
