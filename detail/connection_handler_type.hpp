#ifndef AGENT_CONNECTION_HANDLER_TYPE_HPP_
#define AGENT_CONNECTION_HANDLER_TYPE_HPP_

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

typedef boost::function<
void(boost::system::error_code const&)
  > connect_handler_type;

typedef boost::function<
void(boost::system::error_code const&, boost::uint32_t)
  > io_handler_type;

#endif
