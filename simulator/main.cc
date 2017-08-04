#include <iostream>
#include "spawn.h"
#include "strings.h"
using namespace std;

int main() {
  /*
  std::string json = "[ \"hello JSON\" ]";
  picojson::value v;
  std::string err = picojson::parse(v, json);
  if (! err.empty()) {
    std::cerr << err << std::endl;
  } else {
    std::cerr << "ok" << std::endl;
  }
  */
  vector<string> command = { "/bin/sleep", "1" };
  string output;
  auto r = Spawn(command, "", 1000, &output);
  cout << "Output: " << output << endl;
  cout << "Result: " << (int)r << endl;
  return 0;
}
