#include "agent/agent_v2.hpp"
#include "agent/placeholders.hpp"
#include "agent/log.hpp"
#include <boost/bind.hpp>
#include <functional>
#include <iostream>

void print_to_stdout(boost::system::error_code const &ec, boost::asio::const_buffer buffer)
{

  if( !ec ) {
    //char const *data = boost::asio::buffer_cast<char const*>(buffer);
    //std::cout.write(data, boost::asio::buffer_size(buffer));
  } else {
    std::cerr << "error: " << ec.message() << "\n";   
  }
}

struct qos_handler 
{
  qos_handler() : cnt(0) {}

  void adjust(quality_config &qos)
  {
    ++cnt;
    if(cnt == 100) {
      std::cout << "---- Adjust read speed to 2k ----\n";
      qos.read_max_bps = 2048;
    }
  }
  int cnt;
};

void monitor(agent_conn_action_t action, boost::uintmax_t total, boost::uint32_t transfered)
{
  using namespace std;
  if(action == agent_conn_action_t::upstream) {
    cout << "upstream ";
  } else {
    cout << "downstream ";
  }
  cout << total << " " << transfered << "\n";
}

int main(int argc, char ** argv)
{
  logger::instance().use_file("wget.log");
  boost::asio::io_service ios;
  auto nop = boost::bind(std::plus<int>(), 0, 0);
  qos_handler qos_hdl;
  http::entity::url url(argv[1]);
  agent_v2 agent(ios);

  if( url.query.path.empty() )
    url.query.path = "/";
  
  http::request req;
  http::get_header(req.headers, "Connection")->value = "close";
  agent.async_request(url, req, "GET", false,
                      boost::bind(&qos_handler::adjust, &qos_hdl, agent_arg::qos),
                      boost::bind(&monitor, agent_arg::orient, agent_arg::total, agent_arg::transfered));

  ios.run();

  return 0;
}
