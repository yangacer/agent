#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "connection_fwd.hpp"
#include "agent_handler_type.hpp"
#include "entity.hpp"

class session_type;

class agent
: private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
  typedef boost::shared_ptr<session_type> session_ptr;
public:
  typedef agent_handler_type handler_type;

  agent(boost::asio::io_service &io_service);
  ~agent();

  void async_get(std::string const &url, bool chunked_callback,
                 handler_type handler, bool async = true);

  void async_get(std::string const &url, 
                 http::entity::query_map_t const &parameter,
                 bool chunked_callback, 
                 handler_type handler,
                 bool async = true);

  void async_post(std::string const &url, 
                  http::entity::query_map_t const &get_parameter,
                  http::entity::query_map_t const &post_parameter,
                  bool chunked_callback, 
                  handler_type handler,
                  bool async = true);

  void async_cancel(bool async = true);
  // TODO int speed() const;
  http::request &request();
  boost::asio::io_service& io_service();
protected:
  http::entity::url init(boost::system::error_code &err, std::string const &method, std::string const &url);
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
  void read_body();
  void handle_read_body(boost::system::error_code const &err, boost::uint32_t length);
  void redirect();
  void notify_header(boost::system::error_code const &err);
  void notify_chunk(boost::system::error_code const &err);
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
  handler_type    handler_;
  boost::int64_t  expected_size_;
  boost::int64_t  current_size_;
  bool            is_canceled_;
};
// TODO upload handler (a.k.a write handler)
// TODO better buffer management
#endif
