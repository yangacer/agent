#include <iostream>
#include <cassert>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/thread.hpp>
#include "agent/agent.hpp"
#include "agent/log.hpp"
#include "agent/version.hpp"

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
    : issue_canceled(false), agent_(ios)
  {}

  ~cancel_handler()
  {
    assert(issue_canceled == true);
  }

  void get(std::string const& url) 
  {
    agent_.async_get(
      url, true, 
      boost::bind(&cancel_handler::handle_response, this, _1,_2,_3,_4));  
  }

  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffers_1 buffers)
  {
    namespace error = boost::asio::error;
    if(!ec) {
      agent_.async_abort();
    } else if (ec == error::operation_aborted) {
      // agent is not aborted successfully until now
      issue_canceled = true; 
    } else {
      std::cerr << "cancel_handler: " << ec.message() << "\n";
    }
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
      // write_buffers_to_stdout(buffers);
    } else if( boost::asio::error::eof == ec ) {
      // do someting meaningful
      // std::cerr << "get_handler: eof\n";
    } else {
      std::cerr  << "get_handler: " << ec.message() << "\n";
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

    } else if(ec) {
      std::cerr << "post_handler: " << ec.message() << "\n";
    }
  }
};

int main()
{
  using http::entity::url;
  using http::entity::field;
  using http::entity::query_map_t;
  using http::entity::query_pair_t;
  
  logger::instance().use_file("basic.log", 20 << 10);
  logger::instance().async_log("version: ", false, AGENT_VERSION_FULL);

  boost::asio::io_service ios;
  agent getter(ios), getter_s(ios), poster(ios);
  get_handler get_hdlr;
  post_handler post_hdlr;
  cancel_handler cancel_hdlr(ios);
  query_map_t get_param, post_param;

  // do 'get' request
  getter.async_get( 
    url("http://www.youtube.com/watch?v=8Q2P4LjuVA8"), true,
    boost::bind(&get_handler::handle_response, &get_hdlr,_1,_2,_3,_4));
  
  // do https 'get' request
  getter_s.async_get( 
    url("https://www.google.com.tw/"), true,
    boost::bind(&get_handler::handle_response, &get_hdlr,_1,_2,_3,_4));
  
  // do 'post' request
  post_param.insert(query_pair_t("encoded", "\r\n1234 6"));
  poster.async_post(
    url("http://www.posttestserver.com/?dir=aceryang"), post_param, false,
    boost::bind(&post_handler::handle_response, &post_hdlr,_1,_2,_3,_4));

  // dispatch cancel_handler
  cancel_hdlr.get("http://www.boost.org/");
  
  ios.run();

  return 0;
}
// XXX How to examing correctness without human aid?
