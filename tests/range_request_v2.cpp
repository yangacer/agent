#include <iostream>
#include <boost/bind.hpp>
#include <agent/agent_v2.hpp>
#include <agent/placeholders.hpp>
#include <agent/log.hpp>

void handle_req(boost::system::error_code const &ec) 
{
  if(ec) {
    std::cerr << ec.message() << "\n";   
  }
}

int main(int argc, char **argv)
{
  logger::instance().use_file("range_request_v2.log");
  boost::asio::io_service ios;
  agent_v2 agent(ios);
  
  http::entity::url url(argv[1]);
  http::request req;

  http::get_header(req.headers, "Range")->value = "bytes=0-25536";
  agent.async_request(url, req, "GET", true, 
                      boost::bind(&handle_req, agent_arg::error));
                            
  ios.run();

  return 0;
}
