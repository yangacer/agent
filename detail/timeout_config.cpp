#include "agent/timeout_config.hpp"

const timeout_config timeout_config::default_config(5, 1, 1);

timeout_config::timeout_config(int c, int wk, int rk)
  : conn_(c), wrt_kb_(wk), rd_kb_(rk)
{
  if(this != &default_config) {
    if(c <= 0)  conn_ = default_config.conn_;
    if(wk <= 0) wrt_kb_ = default_config.wrt_kb_;
    if(rk <= 0) rd_kb_ = default_config.rd_kb_;
  }
}

int timeout_config::connect() const
{ return conn_; }

int timeout_config::write_num_bytes(boost::uint32_t bytes) const
{ return (bytes < 1024) ? wrt_kb_ : wrt_kb_ * (bytes >> 10); }

int timeout_config::read_num_bytes(boost::uint32_t bytes) const
{ return (bytes < 1024) ? rd_kb_ : rd_kb_ * (bytes >> 10); }

void timeout_config::connect(int c)
{ if(c > 0) conn_ = c; }

void timeout_config::write_kb(int w)
{ if(w > 0) wrt_kb_ = w; }

void timeout_config::read_kb(int r)
{ if(r > 0) rd_kb_ = r; }
