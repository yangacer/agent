#ifndef AGENCY_HANDLER_TYPE_HPP_
#define AGENCY_HANDLER_TYPE_HPP_

#include <boost/function.hpp>
#include "agent/entity.hpp"
#include "agent/connection_fwd.hpp"

class agency;

typedef boost::shared_ptr<void> session_token_type;

typedef boost::function<
  void(
    boost::system::error_code const &,
    http::request const &, 
    agency &,
    session_token_type)
  > agency_handler_type;

#endif
