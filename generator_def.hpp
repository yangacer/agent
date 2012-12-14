#ifndef GAISWT_GENERATOR_DEF_HPP_
#define GAISWT_GENERATOR_DEF_HPP_

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include "generator.hpp"
#include "fusion_adapt.hpp"

#ifdef GAISWT_DEBUG_GENERATOR  
#include <iostream>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

namespace phoenix = boost::phoenix;
#define GAISWT_DEBUG_GENERATOR_GEN(X) \
    start.name(X); \
    debug(start); 
#else // GAISWT_DEBUG_GENERATOR
#define GAISWT_DEBUG_GENERATOR_GEN(X)
#endif // GAISWT_DEBUG_GENERATOR


BOOST_FUSION_ADAPT_ADT(
  http::entity::uri,
  (std::string, std::string, obj.path, /**/)
  (bool, bool, obj.query_map.empty(), /**/)
  (http::entity::query_map_t, http::entity::query_map_t, obj.query_map, /**/)
  )

namespace http{ namespace generator {

template<typename Iterator>
url_esc_string<Iterator>::url_esc_string()
: url_esc_string::base_type(start)
{
  using karma::char_;
  using karma::eps;
  using karma::_r1;

  start =
    *( char_("a-zA-Z0-9_.~!*'()") |
       char_('-') |
       ( eps(_r1) << char_('/')) |
       ('%' << hex)
     )
    ;
}

template<typename Iterator>
field<Iterator>::field()
: field::base_type(start)
{
  start =
    +karma::char_ << ": " << +karma::char_
    ;
}

template<typename Iterator>
uri<Iterator>::uri()
: uri::base_type(start)
{
  query_value =
    karma::double_ | int64_ | esc_string(ESCAPE_PATH_DELIM)
    ;
  
  query_pair =
    esc_string(ESCAPE_PATH_DELIM) << '=' <<
    query_value
    ;

  query_map =
    (query_pair % '&')
    ;

  start =
    esc_string(RESERVE_PATH_DELIM) <<
    // false if query_map is not empty
    (&karma::false_ <<  '?' | karma::skip[karma::bool_]) <<
    -query_map
    ;

}

template<typename Iterator>
response<Iterator>::response()
: response::base_type(start)
{
  using karma::int_;

  char const* crlf("\r\n");
  char const sp(' ');

  start =
    karma::lit("HTTP/") << 
    int_ << '.' << int_ << sp << 
    karma::uint_ << sp <<
    +(karma::char_) << crlf <<
    (header % crlf) << crlf
    ;
}

template<typename Iterator>
request<Iterator>::request()
: request::base_type(start)
{
  using karma::int_;

  char const* crlf("\r\n");
  char const sp(' ');

  start =
    +karma::char_ << sp << query << sp <<
    karma::lit("HTTP/") <<
    int_ << '.' << int_ << crlf <<
    (header % crlf) << crlf << crlf
    ;
}

}} // namespace http::generator

#endif // header guard
