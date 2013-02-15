#include <iostream>
#include <fstream>
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
      std::cerr << "eof\n";
    } else {
      std::cerr << "error: " << http::find_header(req.headers, "Host")->value << ": " 
        << ec.message() << "\n";     
    }
  }
};

int main()
{
  using http::entity::url;

  boost::asio::io_service io_service;
  agent getter(io_service);
  get_handler handler;
  logger::instance().use_file("redirect.log");

  getter.async_get(url("http://ookon_web.nuweb.cc:30004/"), true, 
                 boost::bind(&get_handler::handle_response, &handler,
                             _1,_2,_3,_4));
  io_service.run();

  return 0;
}
