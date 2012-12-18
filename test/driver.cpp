#include <boost/bind.hpp>
#include "agent/agent.hpp"
#include "read_some.hpp"

int main(int argc, char** argv)
{
  boost::asio::io_service ios;
  agent a(ios);
  handler_read_some hdlr;

  a("http://www.boost.org/LICENSE_1_0.txt", hdlr.request_, 
    boost::bind(&handler_read_some::handle_response, &hdlr, _1, _2, _3, _4));
  
  ios.run();

  return 0;
}
