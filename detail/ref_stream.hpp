#ifndef _REF_STREAM_HPP
#define _REF_STREAM_HPP

#include <string>
#include "container_device.hpp"
#include <boost/iostreams/stream.hpp>

typedef container_device<std::string> string_device;
typedef boost::iostreams::stream<string_device> ref_str_stream;

#endif //header guard
