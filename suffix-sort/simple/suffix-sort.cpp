#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#include "suffix-sort.h"
#include "util.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

namespace SimpleSuffixSort {

  struct SuffixLess {
    const u8* data;
    const u32 len;

    SuffixLess(const u8* data, const u32 len)
      : data(data), len(len) {}

    bool operator()(const u32 i1, const u32 i2) {
      const u8* s1 = data + i1;
      const u32 len1 = len - i1;
      const u8* s2 = data + i2;
      const u32 len2 = len - i2;

      const u32 min_len = std::min(len1, len2);

      // Note that memcmp treats bytes as unsigned char, which is what we want.
      int cmp = memcmp(s1, s2, (size_t)min_len);

      // Shorter string is considered less if strings match up to shorter string,
      // according to "normal" suffix sort practice.
      return cmp == 0 ? (len1 < len2) : (cmp < 0);
    }
  }; // struct SuffixLess

  u32 common_prefix_len(const u8* data, const u32 len, const u32 i1, const u32 i2) {
      const u8* s1 = data + i1;
      const u32 len1 = len - i1;
      const u8* s2 = data + i2;
      const u32 len2 = len + i2;

      const u32 min_len = std::min(len1, len2);

      u32 prefix_len = 0;
      while(prefix_len < min_len && s1[prefix_len] == s2[prefix_len]) {
	prefix_len++;
      }

      return prefix_len;
  }

} // namespace SimpleSuffixSort

int simple_suffix_sort(const u8* data, const u32 len, u32* SA) {
  // Initialise the suffix array SA with normal order
  for(u32 i = 0; i < len; i++) {
    SA[i] = i;
  }

  // Sort using SuffixLess ordering
  std::sort(SA, SA+len, SimpleSuffixSort::SuffixLess(data, len));

  return SUFFIX_SORT_OK;
}

int simple_lcp(const u8* data, const u32 len, const u32* SA, u32* LCP) {
  if(len == 0) {
    return LCP_OK;
  }

  LCP[0] = 0;
  for(u32 index = 1; index < len; index++) {
    LCP[index] = SimpleSuffixSort::common_prefix_len(data, len, SA[index-1], SA[index]);
  }

  return LCP_OK;
}

int simple_suffix_sort_with_lcp(const u8* data, const u32 len, u32* SA, u32* LCP) {
  int rc;

  rc = simple_suffix_sort(data, len, SA);
  if(rc != SUFFIX_SORT_OK) {
    return rc;
  }

  rc = simple_lcp(data, len, SA, LCP);

  return rc;
}

#ifdef SIMPLE_SUFFIX_SORT_MAIN

// std::string
#include <string>

// exit()
#include <cstdlib>

int main(int argc, char* argv[]) {
  printf("Hallo RPJ\n");

  const char* data;
  u32 len;
  bool show_suffixes = false;

  if(argc <= 1) {
    // data = "banana";
    data = "abracadabra banana abracadabra";
    show_suffixes = true;
    len = strlen(data);
  } else {
    std::string filename = argv[1];
    std::string s = Util::slurp(filename);
    data = s.c_str();
    len = s.length();
  }
  

  printf("Using data string of length %u bytes\n", len);

  u32* SA = new u32[len];
  u32* LCP = new u32[len];

  auto t0 = Time::now();
  
  int rc = simple_suffix_sort_with_lcp((const u8*)data, len, SA, LCP);

  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Suffix array (SA) sort with least-common-prefix (LCP) of data string length %u bytes in %7.3lfms\n", len, secs1*1000.0);
  
  if(rc) {
    fprintf(stderr, "simple_suffix_sort_with_lcp failed with rc %d\n", rc);
    exit(1);
  }

  if(show_suffixes) {
    printf("data: %.*s\n\n", len, data);

    for(u32 i = 0; i < len; i++) {
      u32 index = SA[i];
      
      printf("%8d [CP %8d]: %.*s\n", i, LCP[i], len-index, data+index);
    }
  }
}

#endif //def SIMPLE_SUFFIX_SORT_MAIN
