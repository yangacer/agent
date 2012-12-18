#ifndef READ_ALL_HPP_
#define READ_ALL_HPP_

#include <cassert>
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/error.hpp>
#include <openssl/md5.h>
#include "handler.hpp"
#include "agent/agent.hpp"
#include "agent/entity.hpp"

struct handler_read_all : handler
{
  handler_read_all(boost::asio::io_service &ios)
    : handler(ios), size(0)
  {}
  
  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    connection_ptr connection)
  {
    if(!ec) {
      auto h = http::find_header(resp.headers, "Content-Length");
      size = h->value_as<size_t>();
      buffer_.reset(new char[size]);
      read(connection);
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  void read(connection_ptr connection)
  {
    connection->read(
      boost::asio::buffer(buffer_.get(), size),
      boost::bind(&handler_read_all::handle_read, this, _1, _2, _3));
  }

  void handle_read(
    boost::system::error_code const &ec,
    boost::uint32_t length,
    connection_ptr connection)
  {
    if(!ec) {
      md5_.resize(16);
      MD5((unsigned char*)buffer_.get(), length, (unsigned char*)&md5_[0]);
      handler::finish();
    } else if( ec == boost::asio::error::eof ) {
      assert ( false && "never reach");
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  boost::shared_ptr<char> buffer_;
  size_t size;
};

#endif
