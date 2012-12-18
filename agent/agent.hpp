#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include "connection.hpp"
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
      http::request &,
      http::response const &, 
      connection_ptr)
    > handler_type;

  agent(boost::asio::io_service &ios);
  ~agent();

  void operator()(std::string const &server, std::string const &port, 
                  http::request &request, handler_type handler);
  
  void operator()(std::string const &url, http::request &request, 
                  handler_type handler);
protected:
  void start_op(std::string const &server, std::string const &port, 
                http::request &request, handler_type handler);
  void handle_connect(boost::system::error_code const &err);
  void handle_write_request(boost::system::error_code const &err,
                            boost::uint32_t len);
  void handle_read_status_line(boost::system::error_code const &err);
  void handle_read_headers(boost::system::error_code const &err);
  void redirect();
  void notify_error(boost::system::error_code const &err);
private:
  boost::asio::io_service &io_service_;
  connection_ptr  connection_;
  http::response  response_;
  http::request   *request_ptr_;
  int             redirect_count_;
  handler_type    handler_;
};
// TODO 
// 3. this agent is a http client specific; do we need to make it
//    more general (write a use case for this)

#endif
