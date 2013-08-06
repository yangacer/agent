#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "agent/agent_v2.hpp"
#include "agent/placeholders.hpp"
#include "agent/parser.hpp"
#include "agent/log.hpp"
#include "agent/status_code.hpp"

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
  tube_agent(asio::io_service &ios, int max_retry, std::string const &file)
    : agent_(ios), received_(0), total_(0),
  max_retry_(max_retry), retry_count_(0),
  previous_delay_(5), blocked_timer_(ios)
  {
    ofs_.open(file.c_str(), std::ios::out | std::ios::binary);
    if(!ofs_.is_open()) 
      throw std::runtime_error("Unable to open file");
  }

  void get(std::string const &url,
           std::string const &itag)
  {
    itag_ = itag;
    page_url_ = url;
    http::request req;
    auto connection = http::get_header(req.headers, "Connection");
    connection->value = "close";

    agent_.async_request(
      url, req, "GET", false,
      boost::bind(&tube_agent::handle_page, this, 
                  agent_arg::error, agent_arg::response, agent_arg::buffer));
  }

  ~tube_agent()
  {
    ofs_.flush();
    ofs_.close();
  }

protected:
  void handle_page(
    sys::error_code const& ec, http::response const &resp, asio::const_buffer buffer)
  {
    using std::cerr;
    using std::string;
    using http::entity::field;

    if(!ec) {
    } else if(eof == ec) {
      if( 200 == resp.status_code ) {
        string url, signature;
        if(get_link(buffer, itag_, url, signature)) {
          url.append("%26signature%3D").append(signature);
          auto beg(url.begin()), end(url.end());
          video_url_.clear();
          http::parser::parse_url_esc_string(beg, end, video_url_);
          http::request req;
          std::stringstream cvt;
          cvt << "bytes=" << received_ << "-";
          req.headers << field("Range", cvt.str());
          agent_.async_request(video_url_, req, "GET", true,
                               bind(&tube_agent::handle_video, this, 
                                    agent_arg::error, agent_arg::response, agent_arg::buffer));

        } else {
          char const* data = asio::buffer_cast<char const*>(buffer);
          std::cerr.write(data, asio::buffer_size(buffer));
          std::cerr << "\n---- no matched itag\n";
        }
      } else if(http::forbidden == resp.status_code) {
        delayed_get();
        std::cerr << "\n---- http error code (" << resp.status_code << "\n";
      }
    } else {
      std::cerr << "get_page error: " << ec.message() << "\n";
    }
  }

  bool get_link(asio::const_buffer &buffer, std::string const &target_itag, 
                std::string &url, std::string &signature)
  {
    using namespace std;

    char const *beg(asio::buffer_cast<char const*>(buffer));
    char const *end(beg + asio::buffer_size(buffer));
    char const *iter;
    
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
    sys::error_code const& ec,
    http::response const &resp, asio::const_buffer buffer)
  {
    size_t size = asio::buffer_size(buffer);
    if(!ec && resp.status_code - 200 < 100) {
      if( total_ == 0 ) {
        auto h = http::find_header(resp.headers, "Content-Length");
        std::cerr << "Content length: " << h->value << "\n\n\n";
        total_ = h->value_as<size_t>();
      }

      received_ += size;
      ofs_.write(asio::buffer_cast<char const *>(buffer), size);
      std::cout << "\033[F\033[J"  << received_ << "/" << total_ << "\n";

    } else if(eof == ec) {
      received_ += size;
      ofs_.write(asio::buffer_cast<char const *>(buffer), size);
      if(received_ < total_) {
        delayed_get();
      } else {
        std::cout.width(12);
        std::cout.fill(' ');
        std::cout << "\033[F\033[J" << received_ << "\n";
        std::cout << "Total received: " << received_ << "\n";
      }
    } else {
      std::cerr << "handle_video error: ec(" << ec.message() << 
        "), response dump: \n";
      std::cerr << resp << "\n";
      delayed_get();
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
      blocked_timer_.async_wait(bind(&tube_agent::handle_delayed_get, this, _1));
      previous_delay_ <<= 1;
      retry_count_++;
    } else {
      std::cerr << "Exceed retry limit. Abort job.\n";
      return;
    }
  }
  
  void handle_delayed_get(boost::system::error_code const& ec)
  {
    if(!ec) {
      agent_.io_service().post(boost::bind(&tube_agent::get, this, page_url_, itag_));
    } else if(ec == boost::asio::error::operation_aborted) {
      std::cerr << "Operation aborted\n";
    } else {
      std::cerr << ec.message() << "\n";
    }
  }
private:
  agent_v2 agent_;
  std::string itag_, page_url_, video_url_;
  size_t received_;
  size_t total_;
  int const max_retry_;
  int retry_count_;
  int previous_delay_;
  asio::deadline_timer blocked_timer_;
  std::ofstream ofs_;
};

int main(int argc, char** argv)
{
  using namespace std;

  if(argc < 4) {
    cerr << "Usage: tube <url> <fromat> <file>\n";
    exit(1);
  }

  try {
    asio::io_service ios;
    tube_agent ta(ios, 10, argv[3]);
    logger::instance().use_file("tube.log");

    ta.get(argv[1], argv[2]);

    ios.run();
  } catch (std::runtime_error &e){
    std::cerr << "Error: " << e.what() << "\n";
  }
  return 0;
}
