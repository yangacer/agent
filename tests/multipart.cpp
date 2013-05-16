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

  multipart mp(qm);

  cout << "Size: " << mp.size() << "\n";
  while(!mp.eof()) {
    mp.write_to(cout);
  }

  return 0;
}
