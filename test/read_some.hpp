#ifndef READ_SOME_HPP_
#define READ_SOME_HPP_

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/error.hpp>
#include <string>
#include "agent/agent.hpp"
#include "agent/entity.hpp"

struct handler_read_some
{
  handler_read_some()
  {
    using http::entity::field;

    request_.method = "GET";
    request_.headers << field("Accept", "*/*") <<
      field("User-Agent", "agent/client") <<
      field("Connection", "close")
    ;
  }

  void handle_response(
    boost::system::error_code const &ec,
    http::request &req,
    http::response const &resp,
    connection_ptr connection)
  {
    if(!ec) {
      read(connection);
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  void read(connection_ptr connection)
  {
    connection->read_some(
      boost::asio::buffer(buffer_, 4096),
      boost::bind(&handler_read_some::handle_read, this, _1, _2, _3));
  }

  void handle_read(
    boost::system::error_code const &ec,
    boost::uint32_t length,
    connection_ptr connection)
  {
    if(!ec) {
      std::cout.write(buffer_, length);
      read(connection);
    } else if( ec == boost::asio::error::eof ) {
      // do something meaningful
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }
  
  char buffer_[4096];
  http::request request_;
};

#endif
