#ifndef AGENT_BASE_HPP_
#define AGENT_BASE_HPP_

#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "agent/connection_fwd.hpp"
#include "agent/agent_handler_type.hpp"
#include "agent/entity.hpp"

class session_type;

class agent_base
: private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
  typedef boost::shared_ptr<session_type> session_ptr;
  typedef boost::shared_ptr<agent_handler_type> handler_ptr;
public:
  typedef agent_handler_type handler_type;
  
  agent_base(boost::asio::io_service &io_service);
  ~agent_base();

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
  // TODO int speed() const;
  http::request &request();
  boost::asio::io_service& io_service();
protected:
  void init(std::string const &method, http::entity::url const &url);
  void start_op(std::string const &server, std::string const &port, 
                handler_type handler);
  void handle_resolve(boost::system::error_code const &err, 
                      tcp::resolver::iterator endpoint);
  void handle_connect(boost::system::error_code const &err);
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
  void notify_header(boost::system::error_code const &err);
  void notify_chunk(boost::system::error_code const &err, boost::uint32_t length = -1);
  void notify_error(boost::system::error_code const &err);
private:
  boost::asio::io_service &io_service_;
  tcp::resolver   resolver_;
  connection_ptr  connection_;
  session_ptr     session_;
  http::response  response_;
  http::request   request_;
  unsigned char   redirect_count_;
  bool            chunked_callback_;
  handler_ptr     handler_;
  boost::uint64_t expected_size_;
  bool            is_redirecting_;
};
// TODO upload handler (a.k.a. write handler)
// TODO better buffer management
// TODO auto retry (within configurable max_retry)
#endif
