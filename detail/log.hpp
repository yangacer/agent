#ifndef AGENT_LOG_HPP_
#define AGENT_LOG_HPP_
#include <ctime>
#include <string>
#include <iostream>

#define AGENT_TIMED_LOG(Name, StreamableData) \
  std::cerr << "---- " << timestamp() << " " Name << "\n"; \
  std::cerr << StreamableData; \
  std::cerr << "---- end of " Name " ----\n"; 

std::string timestamp(time_t time=0);

#endif
