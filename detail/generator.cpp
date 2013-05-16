#include "generator.ipp"
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/streambuf.hpp>

#define GAISWT_EXP_INST_GENERATORS(IterType) \
  template url_esc_string<IterType>::url_esc_string(); \
  template field<IterType>::field(); \
  template response<IterType>::response(); \
  template request<IterType>::request(); \
  template uri<IterType>::uri(); \
  template query_value<IterType>::query_value(); \
  template query_map<IterType>::query_map(); 

namespace http { namespace generator {
  
  GAISWT_EXP_INST_GENERATORS(ostream_iterator);
  GAISWT_EXP_INST_GENERATORS(std::string::iterator);

  GAISWT_EXP_INST_GENERATORS(
    boost::asio::buffers_iterator<
      boost::asio::streambuf::mutable_buffers_type
      >
    );

}} // namespace http::generator
