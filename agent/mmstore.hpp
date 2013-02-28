#ifndef AGENT_MMSTORE_HPP_
#define AGENT_MMSTORE_HPP_

#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>

class mmstore_impl;

class mmstore
{
public:
  typedef unsigned int handle_type;
  typedef boost::uint64_t offset_type;
  typedef boost::uint64_t size_type;

  struct page_type
  {
    offset_type offset;
    size_type committed;
    boost::asio::mutable_buffer buffer;
  };

  typedef boost::function<
    void(boost::system::error_code const &,
         handle_type,
         page_type page)
    > handler_type;
  
  void configure(size_type page_number = 128 << 20, size_type page_size = 64 << 10);
  
  // TODO Asynchronous ?
  handle create(std::string const &path, size_type size=0);
  handle import(std::string const &path);
  
  void del(std::string const &path);
  void async_map(handle_type handle, offset_type offset, handler_type handler);
  void async_commit(handle_type handle, page_type page, handler_type handler);
  void async_read(handle_type handle, offset_type offset, handler_type handler);

private:
  mmstore();
  static std::unique_ptr<mmstore> instance_;
  static boost::mutex mutex_;
  boost::shared_ptr<mmstore_impl> impl_;
};

#endif
