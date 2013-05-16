#include <string>
#include <boost/bind.hpp>
#include "detail/agent_base_v2.hpp"
#include "agent/version.hpp"
#include "agent/log.hpp"

#define USER_AGENT_STR "OokonHTTPAgent Version"

http::request gen_request(http::entity::url const &url, std::string const &method)
{
  http::request request_;
  auto host = http::get_header(request_.headers, "Host");
  request_.method = method;
  request_.http_version_major = 1;
  request_.http_version_minor = 1;
  request_.query = url.query;
  host->value = url.host;
  if(url.port) 
    host->value += ":" + boost::lexical_cast<std::string>(url.port);

  auto &headers = request_.headers;
  auto header = http::get_header(headers, "Accept");
  if(header->value.empty()) header->value = "*/*";
  header = http::get_header(headers, "Connection");
  if(header->value.empty()) header->value = "keep-alive";
  header = http::get_header(headers, "User-Agent");
  if(header->value.empty()){
    std::string ustr = USER_AGENT_STR;
    ustr += "/";
    ustr += agent_version();
    header->value = ustr;
  }
  return request_;
}

void null_cb(
  boost::system::error_code const &, http::request const&,
  http::response const&,
  boost::asio::const_buffer)
{
  
}

int main(int argc, char **argv)
{
  using namespace std;
  logger::instance().use_file("/dev/stderr");

  boost::asio::io_service ios;
  agent_base_v2 agent(ios);

  http::entity::url url = string(argv[1]);
  http::request req = gen_request(url, "GET");

  agent.async_get(
    url, req, false, 
    boost::bind(&null_cb, _1,_2,_3,_4));

  ios.run();
  return 0;
}
