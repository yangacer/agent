#include <string>
#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include "agent/agent_v2.hpp"
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
    std::string pname = file.substr(file.find_last_of("/") +1);
    std::stringstream literal_url;

    literal_url << "http://127.0.0.1:8000/1Path/CreateFile?" <<
      "partial_name=" << pname << 
      "&%40file=" << file <<
      "&file_atime=" << "2013-05-20T07%3A40%3A47.448" <<
      "&file_mtime=" << "2013-05-20T07%3A40%3A47.448" <<
      "&file_ctime=" << "2013-05-20T07%3A40%3A47.448"
      ;

    http::entity::url url(literal_url.str());
    boost::shared_ptr<string> accumulated(new string);;
    agent_.async_request(
      url, request_, "POST", true, 
      boost::bind(&uploader::handle_post, this, _1, _2, _3, _4, accumulated));
  }

  void handle_post(
    boost::system::error_code const &ec,
    http::request const &req, http::response const &resp,
    boost::asio::const_buffer buf,
    boost::shared_ptr<std::string> accumulated)
  {
    if(ec && ec != boost::asio::error::eof) {
      std::cerr << "Error: " << ec.message() << "\n";
      return;
    }
    char const *data = boost::asio::buffer_cast<char const*>(buf);
    accumulated->append(data, boost::asio::buffer_size(buf));
    if( ec == boost::asio::error::eof ) {
      std::cout << *accumulated;
    }
  }

private:
  boost::asio::io_service &io_service_;
  //std::string nucs_host_;
  http::request request_;
  agent_v2 agent_;
};

struct login
{
  login(boost::asio::io_service &ios)
    : agent_(ios)
    {}
 
  void start()
  {
    http::entity::url url("http://10.0.0.185:8000/Node/User/Auth");
    http::request req;
  
    url.query.query_map.insert(std::make_pair("name","admin"));
    url.query.query_map.insert(std::make_pair("password","admin"));

    agent_.async_request(url, req, "GET", true, 
                         boost::bind(&login::handle_login, this, _1, _2, _3, _4));
  }

  void handle_login(
    boost::system::error_code const &ec,
    http::request const &req, http::response const &resp,
    boost::asio::const_buffer buf)
  {
    if(!ec || ec == boost::asio::error::eof ) {
      char const *data = boost::asio::buffer_cast<char const*>(buf);
      std::cout.write(data, boost::asio::buffer_size(buf));
    }

    if(ec == boost::asio::error::eof )
      std::cout << resp << "\n";
  }

  agent_v2 agent_;
};

int main(int argc, char **argv)
{
  using namespace std;
  logger::instance().use_file("agent_v2.log");

  boost::asio::io_service ios;
  //uploader upd(ios);
  login lin(ios);

  lin.start();
  //upd.start(argv[1]);

  ios.run();
  return 0;
}
