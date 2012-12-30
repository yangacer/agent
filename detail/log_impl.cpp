#include "log_impl.hpp"
#include <cassert>
#include <boost/bind.hpp>

logger_impl::logger_impl()
{
  if(!thread_) {
    boost::unique_lock<boost::mutex> lock(mutex_);
    io_service_.reset(new boost::asio::io_service);
    work_.reset(new boost::asio::io_service::work(*io_service_));
    if(!thread_) {
      thread_.reset(new boost::thread(
          boost::bind(
            &logger_impl::run, this)));
      thread_->detach();
    }
  }
}

void logger_impl::destroy()
{
  work_.reset();
  if(io_service_.get()) {
    if(thread_.get()) {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if( thread_->joinable() ) {
        thread_->join();
      } 
      assert(0 == io_service_->run_one());
      thread_.reset();
    }
    io_service_.reset();
  }
}

boost::asio::io_service& logger_impl::io_service()
{ return *io_service_; }

void logger_impl::use_file(std::string const &filename)
{
  if(!os_) 
    os_.reset(new std::ofstream(filename));
  else { 
    if (os_->is_open()) os_->close();
    os_->open(filename);
  }
}

void logger_impl::async_log(boost::shared_ptr<std::string> data)
{
  if(!os_.get()) return;
  (*os_) << *data ;
  data.reset();
  os_->flush();
}

void logger_impl::run()
{
  // XXX It's tricky to setup a lock here. Though, this is the
  // only way I know to keep log finish its tasks.
  boost::unique_lock<boost::mutex> lock(mutex_);
  io_service_->run();
}
