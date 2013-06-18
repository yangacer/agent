#include <iostream>
#include <string>
#include "agent/parser.hpp"

int main(int argc, char **argv)
{
  using namespace std;
  using namespace http::parser;

  if(argc < 2) {
    cout << "Usage: parser <parser_name>\n"
      " This test reads data from stdin, parses according to given parser_name, and show parsed result.\n"
      ;
  }
  string parser(argv[1]);
  string input;
  uint32_t off = 0;
  input.resize(1024);
  while(cin.read(&input[off], input.size() - off)) {
    off += cin.gcount();
    input.resize(input.size() << 1);
  }
  input.resize(off + cin.gcount());
  auto beg(input.begin()), end(input.end());
  bool result = false;
  if(parser == "response") {
  } else if(parser == "request") {
  } else if(parser == "header") {
    std::vector<http::entity::field> header_list;
    result = parse_header_list(beg, end, header_list);
  } else {
    cerr << "Unknown parser: " << parser << "\n";
  }

  if(result)
    cout << "Parse successed\n";
  else
    cout << "Failed\n";

  cout << "Remain input:\n" << input.substr(beg - input.begin());

  return 0;
}
