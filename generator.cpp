#include "generator_def.hpp"
#include <boost/asio.hpp>

#define GAISWT_EXP_INST_GENERATORS(IterType) \
  template url_esc_string<IterType>::url_esc_string(); \
  template field<IterType>::field(); \
  template response<IterType>::response(); \
  template request<IterType>::request(); \
  template uri<IterType>::uri(); 

namespace http { namespace generator {
  
  GAISWT_EXP_INST_GENERATORS(ostream_iterator);
  GAISWT_EXP_INST_GENERATORS(std::string::iterator);

  GAISWT_EXP_INST_GENERATORS(
    boost::asio::buffers_iterator<
      boost::asio::streambuf::mutable_buffers_type
      >
    );

}} // namespace http::generator
