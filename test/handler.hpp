#ifndef TEST_HANDLER_HPP_
#define TEST_HANDLER_HPP_

#include <boost/function.hpp>
#include "agent/agent.hpp"

struct handler 
{
  typedef boost::function<void(std::string const &)> fini_handler_type;
  
  handler(boost::asio::io_service &ios)
  : ios_(ios)
  {}

  boost::asio::io_service& io_service()
  { return ios_; }
  
  void on_finish(fini_handler_type handler)
  {
    handler_ = handler;  
  }

protected:
  void finish()
  {
    ios_.post(boost::bind(handler_, md5_));
  }

  std::string md5_;
  boost::asio::io_service &ios_;
  fini_handler_type handler_;
};

#endif
