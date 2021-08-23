#include "util.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <sstream>

#include <byteswap.h>

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

// Should be in a C source file...

bool suffix_less(const u8* data, const u32 len, const u32 i1, const u32 i2) {
  const u8* s1 = data + i1;
  const u32 len1 = len - i1;
  const u8* s2 = data + i2;
  const u32 len2 = len - i2;

  const u32 min_len = std::min(len1, len2);

  // Fast path - do one u64 (8-byte) comparison before resorting to memcmp.
  // This assumes misaligned u64 memory access.
  // Note bswap_64() to correct for x86 endinaness.
  // No performance advantage :(
  if(false && min_len >= 8) {
    u64 prefix1 = bswap_64(u64_at_offset(s1, 0));
    u64 prefix2 = bswap_64(u64_at_offset(s2, 0));

    if(prefix1 != prefix2) {
      return prefix1 < prefix2;
    }

    // If the prefixes are equal then fall thru to memcmp..
  }

  // Note that memcmp treats bytes as unsigned char, which is what we want.
  int cmp = memcmp(s1, s2, (size_t)min_len);

  // Shorter string is considered less if strings match up to shorter string,
  // according to "normal" suffix sort convention.
  return cmp == 0 ? (len1 < len2) : (cmp < 0);
}

int n_cpp_std_sorts = 0;

void cpp_std_sort_suffixes(const u8* data, const u32 len, u32* suffix_indexes, u32 n_suffixes) {
  if(n_suffixes <= 1) {
    // Nothing to do - it's already trivially sorted...
    return;
  }

  n_cpp_std_sorts++;

  // Sort using SuffixLess ordering
  // std::stable_sort is substantially faster than std::sort :shrug:
  std::stable_sort(suffix_indexes, suffix_indexes + n_suffixes, Util::SuffixLess(data, len));
}
    
