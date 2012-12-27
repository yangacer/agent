#ifndef AGENT_LOG_HPP_
#define AGENT_LOG_HPP_
#include <ctime>
#include <string>
#include <iostream>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#define AGENT_TIMED_LOG(Name, StreamableData) \
  std::cerr << "---- " << timestamp() << " " Name << "\n"; \
  std::cerr << StreamableData; \
  std::cerr << "---- end of " Name " ----\n"; 

std::string timestamp(time_t time=0);

class logger_impl;

class logger
{
public:
  ~logger();
  static logger& instance();
  void use_file(std::string const& filename);
  template<typename StreamableObject>
  void async_log(std::string const &name, StreamableObject const &o);

  template<typename S1, typename S2>
  void async_log(std::string const &name, S1 const &o1, S2 const &o2); 
private:
  logger();
  static std::unique_ptr<logger> instance_;
  static boost::mutex mutex_;
  boost::shared_ptr<logger_impl> impl_;
};

#include "detail/log.ipp"

#endif
