#include "log.hpp"
#include <cstdlib>

#ifdef _MSC_VER
#define gmtime_r(x,y) gmtime_s(y,x)
#endif

std::string timestamp(time_t time)
{
  std::string fmt("[yyyy-mm-dd hh:MM:SS] ");
  std::string rt;
  struct tm tm_;
 
  if(!time) time = std::time(NULL);

  if(gmtime_r(&time, &tm_)){
    rt.resize(fmt.size());
    strftime(&rt[0], fmt.size(), "[%Y/%m/%d %H:%M:%S]", &tm_);
    // erase \0
    rt.resize(rt.size()-1); 
  }

  return rt;
}
