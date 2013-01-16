#ifndef AGENT_LOG_HPP_
#define AGENT_LOG_HPP_
#include <ctime>
#include <string>
#include <memory>
#include <ostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

std::string timestamp();

class logger_impl;

class logger
{
public:
  ~logger();
  static logger& instance();
  void use_file(std::ostream &os);
  void async_log(std::string const &name);

  template<typename StreamableObject>
  void async_log(std::string const &name, bool block, StreamableObject const &o);

  template<typename S1, typename S2>
  void async_log(std::string const &name, bool block, S1 const &o1, S2 const &o2); 
private:
  logger();
  static std::unique_ptr<logger> instance_;
  static boost::mutex mutex_;
  boost::shared_ptr<logger_impl> impl_;
};

#include "detail/log.ipp"

#endif
