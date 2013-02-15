#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include "agent/agent.hpp"
#include "openssl/md5.h"

struct get_last_half
{
  get_last_half(boost::asio::io_service &ios)
  : agent_(ios)
  {
    
    agent_.async_get(
      http::entity::url("http://www.boost.org/LICENSE_1_0.txt"), true,
      669, -1,
      boost::bind(
        &get_last_half::handle, this, _1,_2,_3,_4));
    MD5_Init(&md5_);
  }

  void handle(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffer buffer)
  {
    using namespace boost::asio;
    if(!ec || ec == boost::asio::error::eof) {
      char const* data = buffer_cast<char const*>(buffer);
      MD5_Update(&md5_, data, buffer_size(buffer));
      //std::cout.write(data, buffer_size(*buffers.begin()));
    } else {
      
    }
  }

  agent agent_;
  std::string md5_should_be_;
  MD5_CTX md5_;
};

int main()
{
  boost::asio::io_service ios;

  std::string md5_should_be = "\x54\x9e\xcd\xf4\x20\x5b\x28\x62\x90\xcf\x08\x7d\x0d\xaf\xe0\xae";
  get_last_half g(ios);

  ios.run();
  
  std::string result;
  result.resize(16);

  MD5_Final((unsigned char*)&result[0], &g.md5_);
  
  assert( result == md5_should_be );

  return 0;
}
