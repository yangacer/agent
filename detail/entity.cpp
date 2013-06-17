#include "agent/entity.hpp"
#include "agent/generator.hpp"
#include "agent/status_code.hpp"
#include "agent/parser.hpp"
#include "agent/version.hpp"

#define USER_AGENT_STR "OokonHTTPAgent"

namespace http {
namespace entity {

url::url(std::string const &escaped_url)
  : port(0)
{
  auto beg(escaped_url.begin()), end(escaped_url.end());
  parser::parse_url(beg, end, *this);
}

request::request()
: http_version_major(1), http_version_minor(1)
{
  auto header = get_header(headers, "Accept");
  header->value = "*/*";
  header = get_header(headers, "Connection");
  header->value = "keep-alive";
  header = get_header(headers, "User-Agent");
  std::string ustr = USER_AGENT_STR;
  ustr += " Ver. ";
  ustr += agent_version();
  header->value = ustr;
}

response::response()
  : http_version_major(0), http_version_minor(0),
  status_code(0)
{}

void request::clear()
{
  method.clear();
  query.path.clear();
  query.query_map.clear();
  http_version_major = 0;
  http_version_minor = 0;
  headers.clear();
}

response
response::stock_response(status_type type)
{
  response resp;
  resp.status_code = static_cast<unsigned int>(type);
  resp.http_version_major = 1;
  resp.http_version_minor = 1;
#define STATUS_CASE(Code, Msg) \
case Code: resp.message=Msg; break;
  
  switch(type){
  STATUS_CASE(ok, "OK");
  STATUS_CASE(created, "Created");
  STATUS_CASE(accepted, "Accepted");
  STATUS_CASE(no_content, "No Content");
  STATUS_CASE(partial_content, "Partial Content");
  STATUS_CASE(multiple_choices, "Multiple Choices");
  STATUS_CASE(moved_permanently, "Moved Permanently");
  STATUS_CASE(found, "Found");
  STATUS_CASE(see_other, "See Other");
  STATUS_CASE(not_modified, "Not Modified");
  STATUS_CASE(bad_request, "Bad Request");
  STATUS_CASE(forbidden, "Forbidden");
  STATUS_CASE(not_found, "Not Found");
  STATUS_CASE(out_of_range, "Requested range not satisfiable");
  STATUS_CASE(internal_server_error, "Internal Server Error");
  STATUS_CASE(not_implemented, "Not Implemented");
  STATUS_CASE(bad_gateway, "Bad Gateway");
  STATUS_CASE(service_unavailable, "Service Unabailable");
  }
  return resp;
}

void response::clear()
{
  http_version_major = 0;
  http_version_minor = 0;
  status_code = 0;
  message.clear();
  headers.clear();
}

} // namespace entity

std::vector<entity::field>::iterator 
find_header(std::vector<entity::field>& headers, std::string const& name)
{
  for(auto i = headers.begin(); i != headers.end(); ++i)
    if(i->name == name)
      return i;
  return headers.end();
}

std::vector<entity::field>::const_iterator 
find_header(std::vector<entity::field> const & headers, std::string const& name)
{
  for(auto i = headers.begin(); i != headers.end(); ++i)
    if(i->name == name)
      return i;
  return headers.end();
}

std::vector<entity::field>::iterator
get_header(std::vector<entity::field>& headers, std::string const& name)
{
  for(auto i = headers.begin(); i != headers.end(); ++i) {
    if(i->name == name)
      return i;
  }
  headers.push_back(entity::field(name, ""));
  return headers.end() - 1;
}

} // namespace http

std::ostream & operator << (std::ostream &os, http::entity::field const &f)
{
  os << f.name << ": " << f.value << "\r\n";
  return os;
}

std::ostream & operator << (std::ostream &os, http::entity::request const &req)
{
  http::generator::ostream_iterator out(os);
  http::generator::generate_request(out, req);
  return os;
}

std::ostream & operator << (std::ostream &os, http::entity::response const &rep)
{
  http::generator::ostream_iterator out(os);
  http::generator::generate_response(out, rep);
  return os;
}

std::vector<http::entity::field> &operator << (
  std::vector<http::entity::field> & headers, 
  http::entity::field const &f)
{
  headers.push_back(f);
  return headers;
}
