#include "log_impl.hpp"
#include "ref_stream.hpp"

template<typename S>
void logger::async_log(std::string const &name, S const& o)
{
  boost::shared_ptr<std::string> data(new std::string);
  ref_str_stream rss(*data);
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

  rss << "----" << timestamp() << " " << name << " ----\n" << o <<
    "---- End of " << name << " ----\n"
    ;
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}

template<typename S1, typename S2>
void logger::async_log(std::string const &name, S1 const &o1, S2 const &o2)
{
  boost::shared_ptr<std::string> data(new std::string);
  ref_str_stream rss(*data);
  rss << "----" << timestamp() << " " << name << " ----\n" << o1 << o2 << 
    "---- End of " << name << " ----\n"
    ;
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}
