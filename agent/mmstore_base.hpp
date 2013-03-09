#ifndef AGENT_MMSTORE_IMPL_HPP_
#define AGENT_MMSTORE_IMPL_HPP_

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "agent/mmstore.hpp"

class mmstore_impl 
: public boost::enable_shared_from_this<mmstore_impl>
{
public:
  typedef mmtore::handle_type handle_type;
  typedef mmstore::offset_type offset_type;
  typedef mmstore::size_type size_type;
  typedef mmstore::handler_type handler_type;

  boost::asio::io_service &io_service();
  void configure(size_type page_number = 128 << 20, size_type page_size = 64 << 10);
  void destroy();

  handle create(std::string const &path, size_type size=0);
  handle import(std::string const &path);
  
  void del(std::string const &path);
  void async_map(handle_type handle, offset_type offset, handler_type handler);
  void async_commit(handle_type handle, page_type page, handler_type handler);
  void async_read(handle_type handle, offset_type offset, handler_type handler);

private:
  boost::mutex mutex_;
  boost::mutex work_mutex_;

  boost::shared_ptr<boost::asio::io_service> io_service_;
  boost::shared_ptr<boost::thread> thread_;
  boost::shared_ptr<boost::asio::io_service::work> work_;
};

#endif
