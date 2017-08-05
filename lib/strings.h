#pragma once
#include <string>
#include <vector>
#include <cstdlib>

std::string JoinString(const std::vector<std::string>& strings,
                       const std::string& separator);

std::string ReadFileOrDie(const std::string& filename);
