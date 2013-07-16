#include <boost/thread.hpp>
#include "agent/log.hpp"

void do_log1()
{
  uint32_t i = 0;
  while(i++ < 0xffffff) {
    logger::instance().async_log(
      "log1 message", true,
      "log1: this is log1\n" "log1: Hi~\n" 
      "log1: Nothing can stop me!\n");
  }
}

void do_log2()
{
  uint32_t i = 0;
  while( i++ < 0xffffff) {
    logger::instance().async_log(
      "log2 message", true,
      "log2: this is log2\n"
      "log2: I want to stop the log1!!!\n");
  }
}

int main()
{
  logger::instance().use_file("logtest.log");

  boost::thread t1(&do_log2), t2(&do_log1);
  
  t1.join();
  t2.join();
  return 0; 
}
