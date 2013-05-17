#include <string>
#include <boost/bind.hpp>
#include "detail/agent_base_v2.hpp"
#include "agent/version.hpp"
#include "agent/log.hpp"

#define USER_AGENT_STR "OokonHTTPAgent Version"

void null_cb(
  boost::system::error_code const &, http::request const&,
  http::response const&,
  boost::asio::const_buffer)
{
  
}

int main(int argc, char **argv)
{
  using namespace std;
  logger::instance().use_file("/dev/stderr");

  boost::asio::io_service ios;
  agent_base_v2 agent(ios);

  http::entity::url url = string(argv[1]);
  http::request req;
  req.method = "GET";
  req.query = url.query;

  agent.async_get(url, req, false, 
    boost::bind(&null_cb, _1,_2,_3,_4));

  ios.run();
  return 0;
}
