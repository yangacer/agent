#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/error.hpp>
#include "agent.hpp"
#include "entity.hpp"

using boost::system::error_code;

struct handler
{
  typedef void result_type;

  void handle_response(
    error_code const &ec, 
    http::response const &resp,
    connection_ptr connection)
  {
    if(!ec) {
      std::cout << "header parsed\n";
      read(connection);
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  void read(connection_ptr connection)
  {
    connection->read_some(
      boost::bind(&handler::handle_read, this, _1, _2, _3));
  }

  void handle_read(
    error_code const &ec,
    boost::uint32_t length,
    connection_ptr connection)
  {
    if(!ec) {
      std::cout << "recv " << length << " bytes\n";
      read(connection);
    } else if( ec == boost::asio::error::eof ) {
      std::cout << "all data received\n";
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }
};

int main(int argc, char** argv)
{
  using http::request;
  using http::entity::field;

  if (argc != 4) {
    std::cout << "Usage: agent <server> <service | port> <uri>\n";
    std::cout << "Example:\n";
    std::cout << "  agent www.boost.org 80 /LICENSE_1_0.txt\n";
    return 1;
  }
  
  // test body
  boost::asio::io_service ios;
  agent a(ios);
  request req;
  handler hdlr;

  req = http::request::stock_request(http::request::stock::GET_PAGE);
  req.query.path = argv[3];
  req.headers << field("Host", argv[1]) << field("User-Agent", "agent/client");

  a(argv[1], argv[2], req, 
    boost::bind(&handler::handle_response, &hdlr, _1, _2, _3));
  
  ios.run();

  std::cout << "agent: finished\n";

  return 0;
}
