#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include "detail/agent_base_v2.hpp"
#include "agent/version.hpp"
#include "agent/log.hpp"

void null_cb(
  boost::system::error_code const &ec, http::request const&,
  http::response const&,
  boost::asio::const_buffer buf)
{
  using namespace boost::asio;
  char const *data = buffer_cast<char const*>(buf);
  if(!ec) {
    std::cout.write(data, buffer_size(buf)); 
  } else if(ec == error::eof) {
    std::cout.write(data, buffer_size(buf)); 
  } else {
    std::cout << "Error: " << ec.message() << "\n";
  }
}

struct uploader
{
  uploader(boost::asio::io_service &ios)//, std::string const &nucs_host)
  : io_service_(ios), //, nucs_host_(nucs_host)
    agent_(ios)
  {}

  void start(std::string const &file)
  {
    using namespace std;
    http::entity::url url("http://127.0.0.1:8000/1Path/CreateFile");
    std::string pname = file.substr(file.find_last_of("/") +1);

    url.query.query_map.insert(make_pair("partial_name", pname));
    url.query.query_map.insert(make_pair("@file", file));
    url.query.query_map.insert(make_pair("file_atime", string("2013-05-20T07:40:47.448")));
    url.query.query_map.insert(make_pair("file_ctime", string("2013-05-20T07:40:47.448")));
    url.query.query_map.insert(make_pair("file_mtime", string("2013-05-20T07:40:47.448")));

    agent_.async_request(
      url, request_, "POST", true, 
      boost::bind(&uploader::handle_post, this, _1, _2, _3, _4));
  }

  void handle_post(
    boost::system::error_code const &ec,
    http::request const &req, http::response const &resp,
    boost::asio::const_buffer buf)
  {
    if(!ec || boost::asio::error::eof == ec) {
      char const *data = boost::asio::buffer_cast<char const*>(buf);
      std::cout.write(data, buffer_size(buf)); 
    } else {
      std::cerr << "Error: " << ec.message() << "\n";
    }
  }

private:
  boost::asio::io_service &io_service_;
  //std::string nucs_host_;
  http::request request_;
  agent_base_v2 agent_;
};

int main(int argc, char **argv)
{
  using namespace std;
  logger::instance().use_file("/dev/stderr");

  boost::asio::io_service ios;
  uploader upd(ios);

  upd.start(argv[1]);

  ios.run();
  return 0;
}
