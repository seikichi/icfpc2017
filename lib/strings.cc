#include "strings.h"
#include <cstdio>
using namespace std;

string JoinString(const vector<string>& strings, const string& separator) {
  string message = "";
  for (int i = 0; i < (int)strings.size(); ++i) {
    if (i != 0)
      message += separator;
    message += strings[i];
  }
  return message;
}

string ReadFileOrDie(const string& filename) {
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == nullptr) {
    fprintf(stderr, "Failed to open file: %s\n", filename.c_str());
    exit(1);
  }
  string str;
  for (;;) {
    char buffer[4096];
    int n = fread(buffer, 1, sizeof(buffer), file);
    if (n == 0)
      break;
    str.insert(str.end(), buffer, buffer + n);
  }
  if (ferror(file)) {
    fprintf(stderr, "Failed to read file: %s\n", filename.c_str());
    exit(1);
  }
  return str;
}

picojson::object StringToJson(const std::string &str) {
  picojson::value v;
  string err = parse(v, str);
  if (!err.empty()) {
    cerr << err << endl;
    throw "Json is invalid";
  }
  return v.get<picojson::object>();
}
