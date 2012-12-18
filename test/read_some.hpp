#ifndef READ_SOME_HPP_
#define READ_SOME_HPP_

#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio/error.hpp>
#include <openssl/md5.h>
#include "handler.hpp"
#include "agent/agent.hpp"
#include "agent/entity.hpp"

struct handler_read_some : handler
{
  handler_read_some(boost::asio::io_service &ios)
  : handler(ios)
  {}

  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    connection_ptr connection)
  {
    if(!ec) {
      MD5_Init(&md5_ctx_);
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
      MD5_Update(&md5_ctx_, (unsigned char*)buffer_, length);
      read(connection);
    } else if( ec == boost::asio::error::eof ) {
      md5_.resize(16);
      MD5_Final((unsigned char*)&md5_[0], &md5_ctx_);
      handler::finish();
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }
  
  MD5_CTX md5_ctx_; 
  char buffer_[4096];
};

#endif
