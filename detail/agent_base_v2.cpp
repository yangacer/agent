#include "detail/agent_base_v2.hpp"
#include <string>
#include <sstream>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "agent/parser.hpp"
#include "agent/generator.hpp"
#include "agent/pre_compile_options.hpp"
#include "agent/log.hpp"
#include "agent/version.hpp"
#include "connection.hpp"
#include "session.hpp"

#define USER_AGENT_STR "OokonHTTPAgent Version"

#ifdef AGENT_ENABLE_HANDLER_TRACKING
#   define AGENT_TRACKING(Desc) \
    logger::instance().async_log(Desc, false, (void*)this);
#else
#   define AGENT_TRACKING(Desc)
#endif

std::string 
determine_service_v2(http::entity::url const & url)
{
  return (url.port) ?
    boost::lexical_cast<std::string>(url.port) :
    url.scheme
    ;
}

namespace asio = boost::asio;
namespace asio_ph = boost::asio::placeholders;
namespace sys = boost::system;

agent_base_v2::context::~context()
{
  AGENT_TRACKING("agent_base_v2::context dtor");
}

agent_base_v2::agent_base_v2(asio::io_service &io_service)
: io_service_(io_service),
  resolver_(io_service)
{
  AGENT_TRACKING("agent_base_v2::ctor");
}

agent_base_v2::~agent_base_v2()
{
  AGENT_TRACKING("agent_base_v2::dtor");
}

void agent_base_v2::async_get(
  http::entity::url const &url,
  http::request const &request,
  bool chunked_callback,
  handler_type handler)
{
  context ctx {
    request, 
    {}, // response
    0,  // redirected count
    chunked_callback, 
    0, // expected size
    false, // is redirecting
    0, // connection pointer
    session_ptr(new session_type(io_service_)),
    handler
    // , {} // multipart_ctx
  };
  auto host = http::find_header(request.headers, "Host");
  if(host == request.headers.end())
    throw std::logic_error("Host field not found in request");

  tcp::resolver::query query(
    url.host, 
    determine_service_v2(url));
  
  context_ptr sp(new context(std::move(ctx)));

  resolver_.async_resolve( query,
    boost::bind(&agent_base_v2::handle_resolve, this, _1, _2, sp));
}

void agent_base_v2::async_post_multipart(
  http::request const &request,
  std::vector<std::string> files,
  handler_type handler)
{
  context ctx {
    request, 
    {}, // response
    0,  // redirected count
    false, // chunked callback
    0, // expected size
    false, // is redirecting
    0, // connection pointer
    session_ptr(new session_type(io_service_)),
    handler
    // , multipart
  };
  //auto content_type = http::get_header(ctx.request.headers, "Content-Type");
  //content_type->value = "multipart/form-data; " + ctx.multipart_ctx.boundary;
}

void agent_base_v2::handle_resolve(
  boost::system::error_code const &err, 
  tcp::resolver::iterator endpoint,
  context_ptr ctx_ptr)
{
  if(!err && endpoint != tcp::resolver::iterator()) {
    AGENT_TRACKING("agent_base::handle_resolve(reconnect) " + endpoint->endpoint().address().to_string());
    ctx_ptr->connection.reset(new connection(io_service_));
    ctx_ptr->session->connect_handler =
      boost::bind(&agent_base_v2::handle_connect, this, _1, ctx_ptr);
    ctx_ptr->connection->connect(endpoint, *(ctx_ptr->session));
  } else {
    ctx_ptr->handler(err, ctx_ptr->request, ctx_ptr->response, asio::const_buffer());
  }
}

void agent_base_v2::handle_connect(
  boost::system::error_code const &err,
  context_ptr ctx_ptr)
{
  AGENT_TRACKING("agent_base::handle_connect");
  if(!err) {
    
  }
}

