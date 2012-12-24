#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "connection_fwd.hpp"
#include "entity.hpp"

class agent
: private boost::noncopyable
{
  typedef boost::asio::ip::tcp tcp;
  typedef tcp::resolver resolver;
public:
  typedef boost::function<
    void(
      boost::system::error_code const &,
      http::request const &,
      http::response const &, 
      boost::asio::const_buffers_1 buffer)
    > handler_type;

  agent(boost::asio::io_service &ios);
  ~agent();

  // TODO Collect similar code in following 3 functions
  void async_get(std::string const &url, bool chunked_callback,
           handler_type handler, bool async = true);

  void async_get(std::string const &url, 
           http::entity::query_map_t const &parameter,
           bool chunked_callback, 
           handler_type handler,
           bool async = true);

  // TODO no get param verison
  void async_post(std::string const &url, 
            http::entity::query_map_t const &get_parameter,
            http::entity::query_map_t const &post_parameter,
            bool chunked_callback, 
            handler_type handler,
            bool async = true);

  // TODO void cancel(bool async = true);
  // TODO int speed() const;
  http::request &request();
  boost::asio::io_service& io_service();
protected:
  // TODO bool canceled() const;
  http::entity::url init(boost::system::error_code &err, std::string const &method, std::string const &url);
  void start_op(std::string const &server, std::string const &port, 
                handler_type handler);
  void handle_connect(boost::system::error_code const &err);
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
  connection_ptr  connection_;
  http::response  response_;
  http::request   request_;
  int             redirect_count_;
  bool            chunked_callback_;
  handler_type    handler_;
  boost::int64_t  expected_size_;
  boost::int64_t  current_size_;
};
// TODO 
// 3. this agent is a http client specific; do we need to make it
//    more general (write a use case for this)

#endif
