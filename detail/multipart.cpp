#include "multipart.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/asio/buffer.hpp>
#include <cassert>
#include "agent/generator.hpp"

#ifdef _WIN32
#define PATH_DELIM "\\"
#else
#define PATH_DELIM "/"
#endif

std::string gen_boundary()
{
  std::string rt("--------Boundary");
  time_t tm = time(NULL);
  unsigned char *ch = (unsigned char*)&tm;
  for(int i=0; i < 4; ++i)
    rt += 'A' + ch[i] % 26;
  return rt;
}

multipart::multipart(http::entity::query_map_t const &query_map)
: boundary_(gen_boundary()), 
  buffer_size_(4096),
  buffer_(buffer_size_, 0),
  size_(0), 
  part_list_(query_map.size()+1), 
  cur_(0)
{
  using http::entity::field;
  size_t last = 0;
  char const *CRLF = "\r\n";
  for(auto i = query_map.begin(); i != query_map.end(); ++i) {
    part_list_[last] = new part;
    part &p = *(part_list_[last++]);
    p.text << "--" << boundary_ << CRLF;
    if( '@' != (i->first)[0] ) {
      p.text << field("Content-Disposition", 
                      "form-data; name=\"" + i->first + "\"");
      p.text << CRLF;
      p.text << i->second;
      p.text << CRLF;
      size_ += p.text.str().size();
    } else {
      std::string
        full_name = boost::get<std::string>(i->second),
        partial_name;
      auto pos = full_name.find_last_of(PATH_DELIM);
      if(pos == full_name.size() - 1)
        throw std::logic_error("Unable to upload dir");
      partial_name = pos == std::string::npos ? 
        full_name : full_name.substr(pos+1)
        ;
      std::stringstream dpos;
      dpos << "form-data; name=\"" <<
        i->first.substr(1) <<
        "\"; filename=\"" << partial_name << "\""
        ;
      p.text << field("Content-Disposition", dpos.str()) <<
        field("Content-Type", "application/octet-stream") << CRLF;
      // TODO content-type
      size_ += p.text.str().size();
      struct stat sb;
      stat(full_name.c_str(), &sb);
      size_ += sb.st_size + 2; // 2 for CRLF
      p.file.open(full_name.c_str(), std::ios::in | std::ios::binary);
      if(!p.file.is_open())
        throw std::logic_error("File not found");
    }
  }
  part_list_[last] = new part;
  part &p = *(part_list_[last]);
  p.text << "--" << boundary_ << "--" << CRLF;
  size_ += p.text.str().size();
}

multipart::~multipart()
{
  for(auto i=part_list_.begin(); i != part_list_.end();++i) {
    delete *i;
    *i = 0;
  }
}

std::string const &multipart::boundary() const
{
  return boundary_;
}

boost::intmax_t multipart::size() const
{
  return size_;
}

void multipart::read(boost::asio::streambuf &sb, boost::intmax_t size)
{
  assert(size > 2 && "mutipart: buffer is too small");
  auto mb = sb.prepare(size);
  boost::intmax_t readcnt = 0;
  char *buf = boost::asio::buffer_cast<char*>(mb);
  while(!eof() && size) {
    part &p = *(part_list_[cur_]);
    if( !p.text.eof() ) {
      p.text.read(buf + readcnt, size); 
      size -= p.text.gcount();
      readcnt += p.text.gcount();
      if(!size) break;
    }
    if( p.file.is_open() ) {
      p.file.read(buf + readcnt, size);
      size -= p.file.gcount();
      readcnt += p.file.gcount();
      if( p.file.eof() && size > 2) {
        memcpy(buf + readcnt, "\r\n", 2);
        size -= 2;
        readcnt += 2;
        p.file.close();
      } else {
        break;
      }
      if(!size) break;
    }
    if( p.eof() ) {
      ++cur_;
    }
  }
  sb.commit(readcnt);
}

bool multipart::eof() const
{
  return cur_ >= part_list_.size() ;
}

bool multipart::part::eof() const
{
  return text.eof() && !file.is_open() ;
}
