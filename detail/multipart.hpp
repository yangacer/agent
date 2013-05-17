#ifndef AGENT_MULTIPART_HPP_
#define AGENT_MULTIPART_HPP_

#include <string>
#include <fstream>
#include <sstream>
#include <boost/asio/streambuf.hpp>
#include "agent/entity.hpp"

class multipart
{
public:
  /** Constrct multipart
   * @param query_map 
   * @codebeg
   * "field" -> "value" for parameters
   * "@filed" -> "filename" for files
   * @endcode
   */
  multipart(http::entity::query_map_t const &query_map);
  ~multipart();
  std::string const &boundary() const;
  boost::intmax_t size() const;
  void read(boost::asio::streambuf &sb, boost::intmax_t size);
  bool eof() const;
private:
  struct part
  {
    std::stringstream text;
    std::ifstream file;
    bool eof() const;
  };
  std::string boundary_;
  std::string::size_type const buffer_size_;
  std::string buffer_;
  boost::intmax_t size_;
  std::vector<part*> part_list_;
  std::vector<part*>::size_type cur_;
};

#endif
