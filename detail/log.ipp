#include "log_impl.hpp"
#include "ref_stream.hpp"

template<typename S>
void logger::async_log(std::string const &name, bool block, S const& o)
{
  std::string data;
  ref_str_stream rss(data);
  rss << "---- [" << timestamp() << "] " << name <<
  (block ? " ----\n" : " ") << o << "\n" << std::flush
  ;
  assert(data.size());
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}

template<typename S1, typename S2>
void logger::async_log(std::string const &name, bool block, S1 const &o1, S2 const &o2)
{
  std::string data;
  ref_str_stream rss(data);
  rss << "---- [" << timestamp() << "] " << name << 
    (block ? " ----\n" : " ") << o1 << 
    (block ? "\n" : " ") << o2 << 
    "\n" << std::flush
    ;
  assert(data.size());
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}
