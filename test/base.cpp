#include <iostream>
#include <cassert>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/thread.hpp>
#include "agent/agent.hpp"
#include "agent/log.hpp"

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

struct cancel_handler
{
  cancel_handler(boost::asio::io_service &ios)
    : agent_(ios), issue_canceled(false)
  {}

  void get(std::string const& url) 
  {
    agent_.async_get(
      url, false, 
      boost::bind(&cancel_handler::handle_response, this, _1,_2,_3,_4));  
  }

  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffers_1 buffers)
  {
    if(!issue_canceled) 
      issue_canceled = true;
    else
      assert(false && "cancel failed");

    agent_.async_cancel();
  }

  bool issue_canceled;
  agent agent_;
};

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
  using http::entity::query_pair_t;

  boost::asio::io_service ios;
  agent getter(ios), getter_s(ios), poster(ios);
  get_handler get_hdlr;
  post_handler post_hdlr;
  cancel_handler cancel_hdlr(ios);
  query_map_t get_param, post_param;

  logger::instance().use_file("base.log");
  // do 'get' request
  // setup parameters
  getter.async_get( 
    "http://www.youtube.com/watch?v=8Q2P4LjuVA8", true,
    boost::bind(&get_handler::handle_response, &get_hdlr,_1,_2,_3,_4));
  
  //do https 'get' request
  getter_s.async_get( 
    "https://www.google.com.tw/", true,
    boost::bind(&get_handler::handle_response, &get_hdlr,_1,_2,_3,_4));
  
  // do 'post' request
  get_param.insert(query_pair_t("dir","aceryang"));
  post_param.insert(query_pair_t("encoded", "\r\n1234 6"));
  poster.async_post( "http://www.posttestserver.com/", get_param, post_param, false,
              boost::bind(&post_handler::handle_response, &post_hdlr,_1,_2,_3,_4));

  // dispatch cancel_handler
  cancel_hdlr.get("http://www.boost.org");
  std::cerr << boost::this_thread::get_id() << ": main thread\n";
  
  ios.run();
  
  std::cerr << "main io_service done\n";
  //logger::instance().~logger();

  return 0;
}

