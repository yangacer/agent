#include <iostream>
#include <boost/asio/buffers_iterator.hpp>
#include "agent/agent.hpp"

struct handler 
{
  void handle_response(
    boost::system::error_code const &ec,
    http::request const &req,
    http::response const &resp,
    boost::asio::const_buffers_1 buffer)
  {
    using namespace boost::asio;

    if(!ec) {
      // FIXME these are buffers, not a sigle buffer
      char const *data = buffer_cast<char const*>(buffer);
      //std::cout.write(data, buffer_size(buffer)) ;
    } else if( boost::asio::error::eof == ec ) {
      char const *data = buffer_cast<char const*>(buffer);
      std::cout << "\n---- eof of size " <<
        buffer_size(buffer) << " ----\n";
    } else {
      std::cerr << ec.message() << "\n";
    }
  }
};

int main()
{
  using http::entity::field;
  using http::entity::query_map_t;

  boost::asio::io_service ios;
  agent a(ios);
  handler hdlr;
  query_map_t param;

  // setup parameters
  param.insert({"v","8Q2P4LjuVA8"});

  // do 'get' request
  a.get( "http://www.youtube.com/watch", param, true,
          boost::bind(&handler::handle_response, &hdlr,_1,_2,_3,_4));

  ios.run();
  return 0;
}

