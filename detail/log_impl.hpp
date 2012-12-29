#ifndef AGENT_LOG_IMPL_HPP_
#define AGENT_LOG_IMPL_HPP_

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

// TODO Write a standalone test case for this
class logger_impl
: public  boost::enable_shared_from_this<logger_impl>
{
public:
  logger_impl();
  void destroy();
  boost::asio::io_service& io_service();
  void use_file(std::string const &filename);
  void async_log(boost::shared_ptr<std::string> data);
  void run();
private:
  bool running_;
  boost::mutex mutex_;
  boost::shared_ptr<boost::asio::io_service> io_service_;
  boost::shared_ptr<boost::thread> thread_;
  boost::shared_ptr<std::ofstream> os_;
  boost::shared_ptr<boost::asio::io_service::work> work_;
};


#endif
