#ifndef GAISWT_PARSER_DEF_HPP_
#define GAISWT_PARSER_DEF_HPP_

#include "parser.hpp"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include "fusion_adapt.hpp"


#ifdef GAISWT_DEBUG_PARSER  
#include <iostream>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

namespace phoenix = boost::phoenix;
#define GAISWT_DEBUG_PARSER_GEN(X) \
    using phoenix::val; \
    using phoenix::construct; \
    using namespace qi::labels; \
    start.name(X); \
    qi::on_error<qi::fail> \
    ( start , \
      std::cout<< \
      val("Error! Expecting ")<< \
      _4<< \
      val(" here: ")<< \
      construct<std::string>(_3,_2)<< \
      std::endl \
    ); \
    debug(start); \

#else // GAISWT_DEBUG_PARSER
#define GAISWT_DEBUG_PARSER_GEN(X)
#endif // GAISWT_DEBUG_PARSER

BOOST_FUSION_ADAPT_STRUCT(
  http::entity::uri,
  (std::string, path)
  (http::entity::query_map_t, query_map)
  )

namespace http { namespace parser {

template<typename Iterator>
url_esc_string<Iterator>::url_esc_string()
: url_esc_string::base_type(start)
{
  unesc_char %=
    (qi::lit('%') >> hex2) |
    qi::char_("a-zA-Z0-9_.~!*'()") | 
    qi::char_('-')
    ;

  start %=  
    *(unesc_char - qi::char_(qi::_r1))
    ; 

  GAISWT_DEBUG_PARSER_GEN("url_esc_string");
}

template<typename Iterator>
field<Iterator>::field()
: field::base_type(start)
{
  using qi::char_;
  using qi::lit;

  char const cr('\r');

  start %= 
    +(char_ - ':') >> lit(": ") >>  
    +(char_ - cr);

  GAISWT_DEBUG_PARSER_GEN("field");
}

template<typename Iterator>
header_list<Iterator>::header_list()
: header_list::base_type(start)
{
  using qi::lit;
  char const *crlf("\r\n");

  start %=
    +(field_rule >> lit(crlf))
      ;

  GAISWT_DEBUG_PARSER_GEN("header_list");
}

template<typename Iterator>
uri<Iterator>::uri()
: uri::base_type(start)
{
  using qi::char_;
  using qi::lit;
  using qi::ushort_;
  using qi::print;

  //qi::real_parser< double, strict_real_policies<double> > real_;
  //typedef qi::int_parser< boost::int64_t > int64_parser;
  //int64_parser int64_;

  query_value =
    real_ | int64_ | esc_string((char const*)"&= #") //+(char_ - char_("&= #"))
    ;

  query_pair %=
    esc_string((char const*)"=") >> '=' >>
    query_value
    ;
  
  query_map %=
    query_pair >> *('&' >> query_pair)
    ;

  start %=
    *(char_('/') >> esc_string((char const*)"?# ")) >> 
    //*(print - char_("?# "))) >> 
    -( '?' >> query_map)
    ;
  
  query_value.name("query_value");
  query_pair.name("query_pair");
  query_map.name("query_map");

  GAISWT_DEBUG_PARSER_GEN("uri");

  debug(query_map);
  debug(query_pair);
  debug(query_value);
}

template<typename Iterator>
url<Iterator>::url()
: url::base_type(start)
{
  using qi::char_;
  using qi::lit;
  using qi::ushort_;
  using qi::alnum;

  start %= 
    +(char_ - ':') >> lit("://") >>
    +(char_ - char_(":/?#")) >>
    -( ':' >> ushort_ ) >> 
    - query >>
    -( '#' >> *char_ )
    ;

  query.name("query");

  GAISWT_DEBUG_PARSER_GEN("url");

}

template<typename Iterator>
response_first_line<Iterator>::response_first_line()
: response_first_line::base_type(start)
{
  using qi::char_;
  using qi::lit;
  using qi::int_;
  using qi::uint_;
  using qi::omit;

  char const cr('\r'), sp(' ');
  char const *crlf("\r\n");

  start %= 
    lit("HTTP/") >> int_ >> '.' >> int_ >> sp >>
    uint_ >> sp >>
    +(char_ - cr) >> lit(crlf) //>>
    //-(headers)  
    ;

  GAISWT_DEBUG_PARSER_GEN("response_first_line");
}

template<typename Iterator>
request_first_line<Iterator>::request_first_line()
: request_first_line::base_type(start)
{
  using qi::char_;
  using qi::lit;
  using qi::int_;

  char const sp(' ');
  char const *crlf("\r\n");

  start %= 
    +(char_ - sp) >> sp >> 
    query >> sp >>
    lit("HTTP/") >> int_ >> '.' >> int_ >> crlf
    // >> -(headers)  
    ;

  GAISWT_DEBUG_PARSER_GEN("request_first_line");
}



}} // namespace http::parser

#endif // header guard
