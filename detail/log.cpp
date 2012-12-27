#include "agent/log.hpp"
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#ifdef _MSC_VER
#define gmtime_r(x,y) gmtime_s(y,x)
#endif

std::string timestamp(time_t time)
{
  std::string fmt("[yyyy-mm-dd hh:MM:SS] ");
  std::string rt;
  struct tm tm_;
 
  if(!time) time = std::time(NULL);

  if(gmtime_r(&time, &tm_)){
    rt.resize(fmt.size());
    strftime(&rt[0], fmt.size(), "[%Y/%m/%d %H:%M:%S]", &tm_);
    // erase \0
    rt.resize(rt.size()-1); 
  }
  return rt;
}

std::unique_ptr<logger> logger::instance_ = nullptr;
boost::mutex logger::mutex_;

logger::logger()
{
  impl_.reset(new logger_impl);
  impl_->start_thread();
}

logger::~logger()
{
  impl_->destroy();
  impl_.reset();
}

logger &logger::instance()
{
  if(!instance_) {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if(!instance_) {
      instance_.reset(new logger);
    }
  }
  return *instance_;
}

void logger::use_file(std::string const &filename)
{
  impl_->io_service().post(
    boost::bind(&logger_impl::use_file, impl_, filename));
}

