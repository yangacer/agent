#include "agent/log.hpp"
#include <cstdlib>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

std::string timestamp()
{
  namespace posix = boost::posix_time;
  posix::ptime now = posix::microsec_clock::local_time();
  return posix::to_iso_string(now);
}

std::unique_ptr<logger> logger::instance_ = nullptr;
boost::mutex logger::mutex_;

logger::logger()
{
  impl_.reset(new logger_impl);
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

void logger::use_file(std::ostream &os)
{
  impl_->io_service().post(
    boost::bind(&logger_impl::use_file, impl_, boost::ref(os)));
}

void logger::async_log(std::string const &name)
{
  boost::shared_ptr<std::string> data(new std::string);
  ref_str_stream rss(*data);
  rss << "---- [" << timestamp() << "] " << name << "\n" << std::flush;
  assert(data->size());
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}
