#include <iostream>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <agent/agent_v2.hpp>
#include <agent/placeholders.hpp>
#include <agent/log.hpp>

bool second = false;

typedef boost::shared_ptr<agent_v2> agent_ptr_t;

void handle_req(boost::system::error_code const &ec, http::entity::url url, agent_ptr_t agent_ptr) 
{
  if(ec) {
    std::cerr << ec.message() << "\n";  
    if(!second && ec == boost::asio::error::eof) {
      second = true;
      http::request req;
      http::get_header(req.headers, "Range")->value = "bytes=25536-45536";
      agent_ptr->async_request(url, req, "GET", true, 
                               boost::bind(&handle_req, agent_arg::error, url, agent_ptr));
    }
  }
}

int main(int argc, char **argv)
{
  logger::instance().use_file("range_request_v2.log");
  boost::asio::io_service ios;
  agent_ptr_t agent_ptr(new agent_v2(ios));
  
  http::entity::url url(argv[1]);
  http::request req;

  http::get_header(req.headers, "Range")->value = "bytes=0-25535";
  agent_ptr->async_request(url, req, "GET", true, 
                           boost::bind(&handle_req, agent_arg::error, url, agent_ptr));
                            
  ios.run();

  return 0;
}
