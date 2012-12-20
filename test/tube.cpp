#include <iostream>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include "agent/agent.hpp"
#include "agent/parser.hpp"

namespace asio = boost::asio;
namespace sys = boost::system;
using boost::bind;
using asio::error::eof;

template<typename Iter>
std::string get_value(
  Iter& beg, Iter& end,
  std::string &field, std::string const &delim)
{
  Iter i;
  std::string rt;
  i = std::find(beg, end, '=');
  if( i != end) {
    field.assign(beg, i);
    beg = i + 1;
    i = std::search(beg, end, delim.begin(), delim.end());
    if( i != end ) {
      rt.assign(beg, i);
      beg = i + delim.size();
    }
  }
  return rt;
}

struct tube_agent
{
  tube_agent(asio::io_service &ios)
    : agent_(ios), received_(0)
    {}

  void start(std::string const &url,
             std::string const &itag)
  {
    itag_ = itag;
    agent_.get(
      url, false, 
      bind(&tube_agent::handle_page, this, _1,_2,_3,_4));
  }

  void handle_page(
    sys::error_code const& ec, http::request const& req,
    http::response const &resp, asio::const_buffers_1 buffers)
  {
    if(!ec) {
      
    } else if(eof == ec) {
      auto beg(asio::buffers_begin(buffers)), 
        end(asio::buffers_end(buffers));
      decltype(beg) iter;
      std::string 
        pattern("\"url_encoded_fmt_stream_map\": \""),
        delim("\\u0026");
      std::string itag, signature, url, field, value;
      char const* data = 0;
      int required = 0;
      iter = std::search(beg, end, pattern.begin(), pattern.end());
      if(iter != end) {
        beg = iter + pattern.size();
        iter = std::find(beg, end, '"');
        if( iter != end ) {
          end = iter;
          //std::cerr.write(&*beg, end - beg);
          while( beg < end ) {
            decltype(end) group_end = std::find(beg, end, ',');
            int required = 0;
            for(int i=0;i<6 && required < 3;++i) {
              value = get_value(beg, group_end, field, delim);
              if( field == "itag") {
                itag = value;
                ++required;
              } else if( field == "sig" ) {
                signature = value;
                ++required;
              } else if ( field == "url" ) {
                url = value;
                ++required;
              }
            }
            if(itag == itag_) break;
            beg = group_end + 1;
          }
        }
      }
      if(itag == itag_) {
        url.append("%26signature%3D").append(signature);
        auto beg(url.begin()), end(url.end());
        http::parser::parse_url_esc_string(beg, end, url_);
        // Post to event queue such that current context
        // of agent_ can be cleanuped
        agent_.io_service().post(
          bind(&tube_agent::get_video, this));
      }
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }
  
  void get_video()
  {
    agent_.get(url_, true, 
               bind(&tube_agent::handle_video, this, _1,_2,_3,_4));
  }

  void handle_video(
    sys::error_code const& ec, http::request const& req,
    http::response const &resp, asio::const_buffers_1 buffers)
  {
    if(!ec) {
      if(resp.status_code == 200) {
        size_t size = asio::buffer_size(buffers);
        if( size == 0 ) {
          auto h = http::find_header(resp.headers, "Content-Length");
          std::cerr << "Content length: " << h->value << "\n\n\n";
          total_ = h->value_as<size_t>();
        } else {
          received_ += size;
          std::cout.setf(std::ios::fixed);
          std::cout.precision(2);
          std::cout.setf(std::ios::showpoint);
          std::cout.width(12);
          std::cout << "\033[F\033[J" << (100*received_)/(double)total_ << " %\n";
        }
      } else {
        std::cerr << resp.status_code << "\n";
      }
    } else if(eof == ec) {
      std::cerr << "---- eof ----\n" <<
        "received: " << received_ << "\n";
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

private:
  agent agent_;
  std::string itag_, url_;
  size_t received_;
  size_t total_;

};

int main(int argc, char** argv)
{
  asio::io_service ios;
  tube_agent ta(ios);
  ta.start("http://www.youtube.com/watch?v=2wBD0Onu6KI", "34");

  ios.run();
  return 0;
}
