#ifndef AGENT_TIMEOUT_CONFIG_HPP_
#define AGENT_TIMEOUT_CONFIG_HPP_

#include <boost/cstdint.hpp>

struct timeout_config
{
  timeout_config(int conn = 5, int wrt_kb = 1, int rd_kb = 1);
  int connect() const;
  int write_num_bytes(boost::uint32_t bytes) const;
  int read_num_bytes(boost::uint32_t bytes) const;
  void connect(int conn);
  void write_kb(int wrt_kb);
  void read_kb(int rd_kb);
private:
  int conn_;
  int wrt_kb_;
  int rd_kb_;
  static const timeout_config default_config;
};

#endif
