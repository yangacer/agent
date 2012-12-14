#ifndef GAISWT_FUSION_ADAPT_HPP_
#define GAISWT_FUSION_ADAPT_HPP_

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include "entity.hpp"

BOOST_FUSION_ADAPT_STRUCT(
  http::entity::field,
  (std::string, name)
  (std::string, value)
  )

BOOST_FUSION_ADAPT_STRUCT(
  http::entity::request,
  (std::string, method)
  (http::entity::uri, query)
  (int, http_version_major)
  (int, http_version_minor)
  (std::vector<http::entity::field>, headers)
  )

BOOST_FUSION_ADAPT_STRUCT(
  http::entity::response,
  (int, http_version_major)
  (int, http_version_minor)
  (unsigned int, status_code)
  (std::string, message)
  (std::vector<http::entity::field>, headers)
  )

BOOST_FUSION_ADAPT_STRUCT(
  http::entity::url,
  (std::string, scheme)
  (std::string, host)
  (unsigned short, port)
  (http::entity::uri, query)
  (std::string, segment)
  )

// entity::uri adaption can not be shared among parser and generator
// Thus, they are defined in *_def.hpp files.

#endif
