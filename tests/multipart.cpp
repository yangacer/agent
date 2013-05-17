#include <iostream>
#include "detail/multipart.hpp"
#include "agent/entity.hpp"

int main(int argc, char **argv)
{
  using namespace std;

  http::entity::query_map_t qm;
  qm.insert(make_pair("param", string("value")));
  qm.insert(make_pair("@file", string(argv[1])));
  qm.insert(make_pair("@file2", string(argv[2])));

  int bufmax = 256;
  for(int i=3; i < bufmax; ++i) {
    multipart mp(qm);

    cout << "Size: " << mp.size() << "\n";
    boost::asio::streambuf sb;
    while(!mp.eof()) {
      mp.read(sb, i);
      cout << &sb;
    }
  }
  return 0;
}
