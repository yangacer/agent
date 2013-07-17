#include "agent/agent_v2.hpp"
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
  http::entity::url url(argv[1]);
  agent_v2 agent(ios);

  if( url.query.path.empty() )
    url.query.path = "/";

  agent.async_request(url, http::request(), "GET", true,
                      nop, 
                      boost::bind(&monitor, _1, _2, _3));

  ios.run();

  return 0;
}
