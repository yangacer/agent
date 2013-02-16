#ifndef AGENCY_HANDLER_TYPE_HPP_
#define AGENCY_HANDLER_TYPE_HPP_

#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>
#include "agent/entity.hpp"
#include "agent/connection_fwd.hpp"

class agency;
class session_type;
typedef boost::shared_ptr<session_type> session_ptr;
typedef boost::weak_ptr<void> session_token_type;

typedef boost::function<
  void(
    boost::system::error_code const &,
    http::request const &, 
    agency &,
    session_ptr)
  > agency_handler_type;

#endif
