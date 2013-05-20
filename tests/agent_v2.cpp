#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include "detail/agent_base_v2.hpp"
#include "agent/version.hpp"
#include "agent/log.hpp"

#define USER_AGENT_STR "OokonHTTPAgent Version"

void null_cb(
  boost::system::error_code const &ec, http::request const&,
  http::response const&,
  boost::asio::const_buffer buf)
{
  using namespace boost::asio;
  char const *data = buffer_cast<char const*>(buf);
  if(!ec) {
    std::cout.write(data, buffer_size(buf)); 
  } else if(ec == error::eof) {
    std::cout.write(data, buffer_size(buf)); 
  } else {
    std::cout << "Error: " << ec.message() << "\n";
  }
}

int main(int argc, char **argv)
{
  using namespace std;
  logger::instance().use_file("/dev/stderr");

  boost::asio::io_service ios;
  agent_base_v2 agent(ios);

  http::entity::url url = string(argv[1]);
  http::request req;

  agent.async_request(url, req, "GET", true, 
    boost::bind(&null_cb, _1,_2,_3,_4));

  ios.run();
  return 0;
}
