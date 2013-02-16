#include <iostream>
#include <cstdio>
#include "agent/agency.hpp"
#include "agent/log.hpp"

int main(int argc, char **argv)
{
  using namespace std;

  if( argc < 4 ) {
    cerr << "Usage: agency <address> <port> <thread_number>\n";
    return 1;
  }

  logger::instance().use_file("agency.log");
  agency a(argv[1], argv[2], atoi(argv[3]));
  
  a.run();

  return 0;
}
