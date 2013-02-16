#ifndef AGENT_CONNECTION_FWD_HPP_
#define AGENT_CONNECTION_FWD_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class connection;
typedef boost::shared_ptr<connection> connection_ptr;
typedef boost::weak_ptr<connection> weak_connection_ptr;

#endif
