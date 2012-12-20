#include "agent/entity.hpp"
#include "agent/generator.hpp"
#include "agent/status_code.hpp"

namespace http {
namespace entity {

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
  switch(type){
  case ok:
    resp.message = "OK";
    break;
  case no_content:
    resp.message = "No Content";
    break;
  case found:
    resp.message = "Found";
    break;
  case bad_request:
    resp.message = "Bad Request";
    break;
  case not_found:
    resp.message = "Not Found";
    break;
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
