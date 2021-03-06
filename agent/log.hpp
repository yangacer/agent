#ifndef AGENT_LOG_HPP_
#define AGENT_LOG_HPP_
#include <ctime>
#include <string>
#include <memory>
#include <ostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/config.hpp>

std::string timestamp();

class logger_impl;

class logger
{
public:
  ~logger();
  static logger& instance();
  void use_file(std::string const& filename, boost::uint32_t max_size = 4 << 20);
  void async_log(std::string const &name);
#ifdef BOOST_NO_VARIADIC_TEMPLATES
  template<typename StreamableObject>
  void async_log(std::string const &name, bool block, StreamableObject const &o);

  template<typename S1, typename S2>
  void async_log(std::string const &name, bool block, S1 const &o1, S2 const &o2); 
#else
  template<typename ...S>
  void async_log(std::string const &name, bool block, S&& ...subjects);
#endif
private:
  logger();
  static std::unique_ptr<logger> instance_;
  static boost::mutex mutex_;
  boost::shared_ptr<logger_impl> impl_;
};

#include "detail/log.ipp"

#endif
