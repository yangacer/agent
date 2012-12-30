#include "parser.ipp"
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/streambuf.hpp>

#define GAISWT_EXP_INST_PARSERS(IterType) \
  template url_esc_string<IterType>::url_esc_string(); \
  template field<IterType>::field(); \
  template response_first_line<IterType>::response_first_line(); \
  template request_first_line<IterType>::request_first_line(); \
  template header_list<IterType>::header_list(); \
  template uri<IterType>::uri(); \
  template url<IterType>::url();

namespace http { namespace parser {
  
  GAISWT_EXP_INST_PARSERS(std::string::iterator);
  GAISWT_EXP_INST_PARSERS(std::string::const_iterator);

  GAISWT_EXP_INST_PARSERS(
    boost::asio::buffers_iterator<
      boost::asio::streambuf::const_buffers_type
      >
    );

}} // namespace http::parser
