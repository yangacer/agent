#include <vector>
#include <boost/bind.hpp>
#include "agent/agent.hpp"
#include "agent/entity.hpp"
#include "read_some.hpp"
#include "read_all.hpp"

std::string page_md5 = "e4224ccaecb14d942c71d31bef20d78c";

std::string hex_md5(std::string const &raw)
{
  std::string rt;
  unsigned char c;
  static unsigned char hex_digits[] = { 
    '0', '1', '2', '3', '4', '5', '6', '7', 
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' 
  };

  if(raw.size() < 16)  return rt; 
  rt.resize(32);
  for(size_t i=0; i!=16; ++i) {
    c = (unsigned char)raw[i]; 
    rt[i*2] = hex_digits[c >> 4];
    rt[i*2+1] = hex_digits[c & 0x0f];
  }
  return rt;
}

struct driver 
{
  driver(boost::asio::io_service &ios)
    : hdlr_read_some(ios),
      hdlr_read_all(ios)
  {
    using boost::bind;
    using http::entity::field;

    agents_.resize(2);
    for(auto &i : agents_) {
      i.reset(new agent(ios));
      i->request().method = "GET";
      i->request().headers << 
        field("Accept", "*/*") <<
        field("User-Agent", "agent/client") <<
        field("Connection", "close")
        ;
    }
    agents_[0]->fetch(
      "http://www.boost.org/LICENSE_1_0.txt",
      bind(&handler_read_some::handle_response, &hdlr_read_some, _1, _2, _3, _4));
    agents_[1]->fetch(
      "http://www.boost.org/LICENSE_1_0.txt",
      bind(&handler_read_all::handle_response, &hdlr_read_all, _1, _2, _3, _4));

    hdlr_read_some.on_finish(
      bind(&driver::fini_handler, this, _1));

    hdlr_read_all.on_finish(
      bind(&driver::fini_handler, this, _1));
  }

  void fini_handler(std::string const &md5)
  {
    assert(page_md5 ==  hex_md5(md5));
  }

  std::vector<boost::shared_ptr<agent> > agents_;
  handler_read_some hdlr_read_some;
  handler_read_all hdlr_read_all;
};

int main(int argc, char** argv)
{
  using http::entity::field;
  using boost::bind;

  boost::asio::io_service ios;
  driver d(ios);
  ios.run();
  
  return 0;
}
