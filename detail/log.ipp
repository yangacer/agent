#include "log_impl.hpp"
#include "ref_stream.hpp"
#include <boost/config.hpp>

#ifdef BOOST_NO_VARIADIC_TEMPLATES

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

#else

namespace detail {

template<>
struct print<>
{
  static void &do_it()(ref_str_stream &os, bool block) const
  { return os; }
};

template<typename Head, typename ... Tail>
struct print 
{
  static void &do_it(ref_str_stream &os, bool block, Head const & head, Tail const &... tail) const
  {
    os << (block ? "\n" : " " ) << head;
    return print<Tail...>::(os, block, std::forward<Tail>(tail)...);
  }
};

} // namespace

template<typename ... S>
void logger::async_log(std::string const &name, bool block, S const &... subjects)
{
  std::string data;
  ref_str_stream rss(data);
  rss << "---- [" << timestamp() << "] " << name;
  print<S...>::do_it(rss, block, std::forward<S>(subjects)...);
  rss << "\n" << std::flush;
  assert(data.size());
  impl_->io_service().post(
    boost::bind(&logger_impl::async_log, impl_, data));
}

#endif
