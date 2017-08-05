#pragma once
#include <string>
#include <vector>
#include <cstdlib>
#include "picojson.h"

std::string JoinString(const std::vector<std::string>& strings,
                       const std::string& separator);

std::string ReadFileOrDie(const std::string& filename);

picojson::object StringToJson(const std::string& str);

std::string AddLengthToJson(const std::string& str);
