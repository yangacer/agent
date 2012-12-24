#include <iostream>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "agent/agent.hpp"
#include "agent/parser.hpp"

namespace asio = boost::asio;
namespace sys = boost::system;
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
    rt.assign(beg, i);
    beg = (i != end) ? i + delim.size() : end ;
  }
  return rt;
}

struct tube_agent
{
  tube_agent(asio::io_service &ios, int max_retry)
    : agent_(ios), received_(0), total_(0),
  max_retry_(max_retry), retry_count_(0),
  previous_delay_(5), blocked_timer_(ios)
  {}

  void get(std::string const &url,
           std::string const &itag)
  {
    itag_ = itag;
    page_url_ = url;
    agent_.async_get(url, false, 
      boost::bind(&tube_agent::handle_page, this, _1,_2,_3,_4));
  }

protected:
  void handle_page(
    sys::error_code const& ec, http::request const& req,
    http::response const &resp, asio::const_buffers_1 buffers)
  {
    using std::cerr;
    using std::string;

    if(!ec) {

    } else if(eof == ec) {
      if( 200 == resp.status_code ) {
        string url, signature;

        if(get_link(buffers, itag_, url, signature)) {
          url.append("%26signature%3D").append(signature);
          auto beg(url.begin()), end(url.end());
          video_url_.clear();
          http::parser::parse_url_esc_string(beg, end, video_url_);
          agent_.async_get(video_url_, true, 
                           bind(&tube_agent::handle_video, this, _1,_2,_3,_4));

        } else {
          auto beg(asio::buffers_begin(buffers)), 
            end(asio::buffers_end(buffers));
          std::cerr.write(&*beg, end - beg);
          std::cerr << "\n---- no matched itag\n";
        }
      } else if(403 == resp.status_code) {
        delayed_get();
        std::cerr << "\n---- http error code (" << resp.status_code << "\n";
      }
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  bool get_link(asio::const_buffers_1 &buffers, std::string const &target_itag, 
                std::string &url, std::string &signature)
  {
    using namespace std;

    auto beg(asio::buffers_begin(buffers)), 
         end(asio::buffers_end(buffers));
    decltype(beg) iter;
    string 
      pattern("\"url_encoded_fmt_stream_map\": \""),
      delim("\\u0026");
    string itag, field, value;
    int required = 0;

    iter = search(beg, end, pattern.begin(), pattern.end());
    if(iter != end) {
      beg = iter + pattern.size();
      iter = find(beg, end, '"');
      if( iter != end ) {
        end = iter;
        while( beg < end ) {
          decltype(end) group_end = find(beg, end, ',');
          required = 0;
          for(int i=0;i<6 && required < 3;++i) {
            value = get_value(beg, group_end, field, delim);
            if(value.empty()) {
              cerr << "parsing error: " << field << "\n";
            }
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
          if(itag == target_itag && required == 3) return true;
          if(group_end == end) break;
          beg = group_end + 1;
        }
      }
    }
    return false;
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
      } else if( resp.status_code == 403 ) {
        delayed_get();
      } else {
        std::cerr << resp.status_code << "\n";
      }
    } else if(eof == ec) {
      if(!received_) 
        delayed_get();
    } else {
      std::cerr << "error: " << ec.message() << "\n";
    }
  }

  void delayed_get() 
  {
    if(retry_count_ < max_retry_) {
      std::cerr << 
        "Oops, we are blocked! "
        "This procedure will take place again after " << 
        previous_delay_ << " seconds.\n";  
      blocked_timer_.expires_from_now(boost::posix_time::seconds(previous_delay_));
      blocked_timer_.async_wait(bind(&tube_agent::get, this, page_url_, itag_));
      previous_delay_ <<= 1;
      retry_count_++;
    } else {
      std::cerr << "Exceed retry limit. Abort job.\n";
      return;
    }
  }
private:
  agent agent_;
  std::string itag_, page_url_, video_url_;
  size_t received_;
  size_t total_;
  int const max_retry_;
  int retry_count_;
  int previous_delay_;
  asio::deadline_timer blocked_timer_;
};

int main(int argc, char** argv)
{
  asio::io_service ios;
  tube_agent ta(ios, 10);
  ta.get("http://www.youtube.com/watch?v=NPoHPNeU9fc", "37");

  ios.run();
  return 0;
}
