#include "agent/io_service_pool.hpp"
#include <boost/bind.hpp>

namespace asio = boost::asio;

io_service_pool::io_service_pool(std::size_t number)
  : next_(0)
{
  while(number) {
    io_service_ptr service(new asio::io_service);
    io_services_.push_back(service);
    number--;
  }
}

io_service_pool::io_service_ptr 
io_service_pool::get_io_service(std::size_t number)
{
  io_service_ptr rt;
  if(number == -1) {
    rt = io_services_[next_];
    next_++;
    next_ %= io_services_.size();
  } else {
    rt = io_services_[number];
  }
  return rt;
}

void io_service_pool::run(initiate_status status)
{
  for(auto i=0u;i<io_services_.size();++i) {
    if(status == WITHOUT_WORK) {
      work_ptr work(new asio::io_service::work(*(io_services_[i])));
      works_.push_back(work);
    }
    thread_group_.create_thread(
      boost::bind(&asio::io_service::run, io_services_[i]));  
  }
  thread_group_.join_all();
}

void io_service_pool::stop()
{
  for(auto i=0u;i<io_services_.size();++i) 
    io_services_[i]->stop();
}

