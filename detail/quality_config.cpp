#include "agent/quality_config.hpp"

int const quality_config::unlimited(0);

quality_config::quality_config(int write_max, int read_max)
: write_max_bps(write_max), read_max_bps(read_max)
{}
