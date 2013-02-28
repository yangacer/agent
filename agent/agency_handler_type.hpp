#ifndef AGENCY_HANDLER_TYPE_HPP_
#define AGENCY_HANDLER_TYPE_HPP_

#include <boost/function.hpp>
#include <boost/asio/buffer.hpp>
#include "agent/entity.hpp"
#include "agent/connection_fwd.hpp"

class agency;
class session_type;
typedef boost::shared_ptr<session_type> session_ptr;

typedef boost::function<
  void(
    boost::system::error_code const &,
    http::request const &, 
    agency &,
    session_ptr,
    boost::asio::const_buffer)
  > agency_handler_type;
// TODO add const_buffer to interface
#endif
