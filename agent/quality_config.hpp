#ifndef AGENT_QUALITY_CONFIG_HPP_
#define AGENT_QUALITY_CONFIG_HPP_

struct quality_config
{
  static int const unlimited;

  /** Construct quality configuration
   * @param write_max_kbps Write at most that many BYTES per second.
   * @param read_max_kbps Read at most that many BYTES per second.
   */
  quality_config(int write_max_bps = 0, int read_max_bps = 0);

  int write_max_bps;
  int read_max_bps;
};

#endif
