#ifndef AGENT_IO_SERVICE_POOL_
#define AGENT_IO_SERVICE_POOL_
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

class io_service_pool
: private boost::noncopyable
{
  typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
  typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
public:
  enum initiate_status
  { WITH_WORK = 0, WITHOUT_WORK = 1 };

  io_service_pool(std::size_t number);
  io_service_ptr get_io_service(std::size_t number = -1);
  void run(initiate_status status = WITH_WORK);
  void stop();
private:
  boost::thread_group thread_group_;
  std::vector<io_service_ptr> io_services_;
  std::vector<io_service_ptr> works_;
  size_t next_;
};

#endif
