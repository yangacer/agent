#include <iostream>
#include "agent/io_service_pool.hpp"
#include "agent/agent.hpp"
#include "agent/log.hpp"

struct get_handler
{
  void handle_response(
    boost::system::error_code const & ec,
    http::request const &req,
    http::response const& resp,
    boost::asio::const_buffer buffer)
  {
    if(!ec) {

    } else if ( ec == boost::asio::error::eof) {

    } else {
      std::cerr << "error: " << http::find_header(req.headers, "Host")->value << ": " 
        << ec.message() << "\n";     
    }
  }
};

int main()
{
  using http::entity::url;

  io_service_pool sp(2);

  agent get1(*sp.get_io_service(0)), 
        get2(*sp.get_io_service(0)),
        get3(*sp.get_io_service(1)),
        get4(*sp.get_io_service(1))
          ;
  get_handler h1, h2, h3, h4;
  
  get1.async_get(url("http://www.google.com/"), true, 
                 boost::bind(&get_handler::handle_response, &h1,
                             _1,_2,_3,_4));

  get2.async_get(url("http://www.youtube.com/"), true, 
                 boost::bind(&get_handler::handle_response, &h2,
                             _1,_2,_3,_4));

  get3.async_get(url("http://ookon_web.nuweb.cc:30004/"), true, 
                 boost::bind(&get_handler::handle_response, &h3,
                             _1,_2,_3,_4));

  get4.async_get(url("http://www.yahoo.com/"), false, 
                 boost::bind(&get_handler::handle_response, &h4,
                             _1,_2,_3,_4));

  sp.run();

  return 0;
}
