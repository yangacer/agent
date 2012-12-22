#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "agent/agent.hpp"

void write_buffers_to_stdout(boost::asio::const_buffers_1 buffers)
{
  using namespace boost::asio;

  const_buffers_1::const_iterator i = buffers.begin();
  char const* data = 0;
  while ( i != buffers.end() ) {
    data = buffer_cast<char const*>(*i);
    std::cout.write(data, buffer_size(*i)) ;
    std::cout.flush();
    ++i;
  }
}

struct get_handler 
{
  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffers_1 buffers)
  {
    using namespace boost::asio;

    if(!ec) {
      //write_buffers_to_stdout(buffers);
    } else if( boost::asio::error::eof == ec ) {
      // do someting meaningful
    } else {
      std::cerr  << ec.message() << "\n";
    }
  }
};

struct post_handler
{
  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffers_1 buffers)
  {
    using namespace boost::asio;
    if( boost::asio::error::eof == ec ) {
      std::cerr << "response code: " << resp.status_code << "\n";
    } else if(ec) {
      std::cerr << ec.message() << "\n";
    }
  }
};

int main()
{
  using http::entity::field;
  using http::entity::query_map_t;

  boost::asio::io_service ios;
  agent getter(ios), poster(ios);
  get_handler get_hdlr;
  post_handler post_hdlr;
  query_map_t get_param, post_param;

  // do 'get' request
  // setup parameters
  get_param.insert({"v","8Q2P4LjuVA8"});
  getter.async_get( 
    "http://www.youtube.com/watch", get_param, true,
    boost::bind(&get_handler::handle_response, &get_hdlr,_1,_2,_3,_4));
  
  // do 'post' request
  get_param.clear();
  get_param.insert({"dir","aceryang"});
  post_param.insert({"encoded", "\r\n1234 6"});
  poster.async_post( "http://www.posttestserver.com/", get_param, post_param, false,
              boost::bind(&post_handler::handle_response, &post_hdlr,_1,_2,_3,_4));
  ios.run();
  return 0;
}

