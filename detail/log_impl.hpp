#ifndef AGENT_LOG_IMPL_HPP_
#define AGENT_LOG_IMPL_HPP_

#include <ostream>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

class logger_impl
: public  boost::enable_shared_from_this<logger_impl>
{
public:
  logger_impl();
  void destroy();
  boost::asio::io_service& io_service();
  void use_file(std::streambuf *rdbuf);
  void async_log(boost::shared_ptr<std::string> data);
  void run();
private:
  bool running_;
  boost::mutex mutex_;
  boost::mutex work_mutex_;
  boost::shared_ptr<boost::asio::io_service> io_service_;
  boost::shared_ptr<boost::thread> thread_;
  boost::shared_ptr<std::ostream> os_;
  boost::shared_ptr<boost::asio::io_service::work> work_;
};


#endif