#ifndef GAISWT_GENERATOR_HPP_
#define GAISWT_GENERATOR_HPP_

#include <boost/spirit/include/karma_grammar.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/support_ostream_iterator.hpp>
#include "entity.hpp"

namespace http {
namespace generator {

namespace karma = boost::spirit::karma;

using boost::spirit::ostream_iterator;

enum path_delim_option{ 
  ESCAPE_PATH_DELIM = false, 
  RESERVE_PATH_DELIM = true
};

template<typename Iterator>
struct url_esc_string
: karma::grammar<Iterator, std::string(path_delim_option)>
{
  url_esc_string();
  karma::rule<Iterator, std::string(path_delim_option)> start;
  karma::uint_generator<unsigned char, 16> hex;
};

template<typename Iterator>
struct field 
: karma::grammar<Iterator, entity::field()>
{
  field();
  karma::rule<Iterator, entity::field()> start;
};

template<typename Iterator>
struct uri
: karma::grammar<Iterator, entity::uri()>
{
  uri();
  karma::rule<Iterator, entity::uri()> start;
  karma::rule<Iterator, entity::query_value_t()> query_value;
  karma::rule<Iterator, std::pair<std::string, entity::query_value_t>()> query_pair;
  karma::rule<Iterator, entity::query_map_t()> query_map;
  url_esc_string<Iterator> esc_string;
  karma::int_generator< boost::int64_t > int64_;
};

template<typename Iterator>
struct response
: karma::grammar<Iterator, entity::response()>
{
  response();
  karma::rule<Iterator, entity::response()> start;
  field<Iterator> header;
};

template<typename Iterator>
struct request
: karma::grammar<Iterator, entity::request()>
{
  request();
  karma::rule<Iterator, entity::request()> start;
  uri<Iterator> query;
  field<Iterator> header;
};

#define GEN_GENERATE_FN(Generator) \
  template<typename Iter, typename Struct> \
  bool generate_##Generator(Iter& out, Struct const &obj) \
  { \
    static Generator<Iter> gen; \
    return karma::generate(out, gen, obj); \
  }

GEN_GENERATE_FN(url_esc_string)
GEN_GENERATE_FN(field)
GEN_GENERATE_FN(uri)
GEN_GENERATE_FN(response)
GEN_GENERATE_FN(request)

}} // namespace http::generator

#endif // header guard
