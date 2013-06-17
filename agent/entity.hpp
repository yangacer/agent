#ifndef GAISWT_ENTITY_HPP_
#define GAISWT_ENTITY_HPP_

#include <string>
#include <ostream>
#include <vector>
#include <map>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include "agent/status_code.hpp"
 
namespace http {
namespace entity {

struct field
{
  field(){}
  
  template<typename T>
  field(std::string name, T value)
  : name(name), value(boost::lexical_cast<std::string>(value))
  {}

  template<typename T>
  T value_as() const
  { return boost::lexical_cast<T>(value); }

  template<typename T>
  void value_as(T val)
  { value = boost::lexical_cast<std::string>(val); }
 
  std::string name, value;
};

typedef boost::variant<boost::int64_t, double, std::string> 
query_value_t;

typedef std::pair<std::string, query_value_t>
query_pair_t;

typedef std::multimap<std::string, query_value_t> 
query_map_t;

struct uri
{
  std::string path;
  query_map_t query_map;
};

struct url
{
  url():port(0){}
  url(std::string const &escpaed_url);

  std::string scheme;
  std::string host;
  unsigned short port;
  uri query;
  std::string segment;
};

struct request
{
  request();
  std::string method;
  uri query;
  int http_version_major,
      http_version_minor;
  std::vector<field> headers;

  void clear();
};

struct response
{
  response();
  int http_version_major,
      http_version_minor;

  unsigned int status_code;
  std::string message;
  std::vector<field> headers;

  static response
  stock_response(status_type s);
  void clear();
};

} // namespace entity

std::vector<entity::field>::iterator 
find_header(std::vector<entity::field>& headers, std::string const& name);

std::vector<entity::field>::const_iterator 
find_header(std::vector<entity::field> const &headers, std::string const& name);

std::vector<entity::field>::iterator
get_header(std::vector<entity::field>& headers, std::string const& name);

typedef entity::response response;
typedef entity::request request;

} // namespace http

std::ostream & operator << (std::ostream &os, http::entity::field const &f);
std::ostream & operator << (std::ostream &os, http::entity::request const &req);
std::ostream & operator << (std::ostream &os, http::entity::response const &rep);
std::vector<http::entity::field> &operator << (
  std::vector<http::entity::field> & headers,
  http::entity::field const &f);

#endif // header guard
