#include "session.hpp"
#include "agent/log.hpp"
#include "agent/pre_compile_options.hpp"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#else
#   define AGENT_TRACKING(Desc)
#endif

session_type::session_type(boost::asio::io_service &io_service)
  : timer(io_service)  
{
  AGENT_TRACKING("session_type::ctor");
  io_buffer.prepare(AGENT_CONNECTION_BUFFER_SIZE);
}

session_type::~session_type()
{
  AGENT_TRACKING("session_type::dtor");
  connect_handler = connect_handler_type();
  io_handler = io_handler_type();
}
