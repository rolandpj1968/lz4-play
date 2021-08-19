#include "util.h"

#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <sstream>

namespace Util {
    
  std::string slurp(const std::string& filepath) {
    std::ifstream t(filepath);
    std::string str;
    
    t.seekg(0, std::ios::end);   
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    
    str.assign((std::istreambuf_iterator<char>(t)),
	       std::istreambuf_iterator<char>());
    
    return str;
  }
  
} // namespace Util
