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
      // according to "normal" suffix sort convention.
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

  // Initialise the suffix array SA with identity order
  void init_sa_identity(u32* SA, const u32 len) {
    for(u32 i = 0; i < len; i++) {
      SA[i] = i;
    }
  }

  int cpp_std_sort_suffixes(const u8* data, const u32 len, u32* suffix_indexes, u32 n_suffixes) {
    // Sort using SuffixLess ordering
    std::sort(suffix_indexes, suffix_indexes + n_suffixes, SimpleSuffixSort::SuffixLess(data, len));
    
    return SUFFIX_SORT_OK;
  }
    
  int cpp_std_sort(const u8* data, const u32 len, u32* SA) {
    init_sa_identity(SA, len);

    return cpp_std_sort_suffixes(data, len, SA, len);
  }

  static const size_t RADIX_BUF_SIZE = 256 + 1; // u8 range plus one extra

  // Stable sort a set of suffixes with common prefix in-place.
  // It's not really in-place but output is in-place.
  // cp_len is known common-prefix length of all suffixes - we will look at the
  //   next char of each suffix for this radix level.
  int radix_sort_level(const u8* data, const u32 len, u32* suffix_indexes,
		       u32 n_suffixes, u32 cp_len, u32 radix_buf[RADIX_BUF_SIZE]) {

    if(n_suffixes <= 1) {
      // Nothing to do - it's already trivially sorted...
      return SUFFIX_SORT_OK;
    }

    // Reset the radix buffer
    for(size_t i = 0; i < RADIX_BUF_SIZE; i++) {
      radix_buf[i] = 0;
    }

    bool found_end_of_str_suffix = false;
      
    // We first use the radix buffer to count instances of each radix
    for(u32 suffix_no = 0; suffix_no < n_suffixes; suffix_no++) {
      u32 suffix_index = suffix_indexes[suffix_no];
      u32 suffix_len = len - suffix_index;

      // Special case - suffix next char is <end-of-string> - there can only be
      // one of these and it will be the first in sorted order. So we handle it
      // specially by swapping it with the first element of suffix_indexes
      // array.
      if(suffix_len == cp_len) {
	u32 suffix_index_0 = suffix_indexes[0];
	suffix_indexes[0] = suffix_index;
	suffix_indexes[suffix_no] = suffix_index_0;

	found_end_of_str_suffix = true;
	
      } else {
	u8 c = data[suffix_index + cp_len];
	// Note we put the count of 'c' into element [c+1] for easy conversion
	// to bucket offsets (below).
	radix_buf[c+1]++;
      }
    }

    // If we found the end-of-string suffix then we've moved it to its correct
    // final position at the beginning of the suffixes and from now on we only
    // have to consider the other suffixes.
    if(found_end_of_str_suffix) {
      suffix_indexes++;
      n_suffixes--;

      if(n_suffixes <= 1) {
	// Nothing to do - it's already trivially sorted...
	return SUFFIX_SORT_OK;
      }
    }

    // Now we convert initial char counts into radix bucket offsets.
    u32 offset = 0;
    for(u32 c = 0; c < 256; c++) {
      offset += radix_buf[c+1];
      radix_buf[c+1] = offset;
    }

    // ... and place each suffix into its radix bucket
    // for now we need a temporary buffer of indexes... ugh
    u32* sorted_suffix_indexes = new u32[n_suffixes];

    for(u32 suffix_no = 0; suffix_no < n_suffixes; suffix_no++) {
      u32 suffix_index = suffix_indexes[suffix_no];
      u8 c = data[suffix_index + cp_len];

      u32 sorted_suffix_offset = radix_buf[c]++;
      sorted_suffix_indexes[sorted_suffix_offset] = suffix_index;
    }

    // ... and copy the buckets back to the original array.
    // We can avoid this copy with alternating temp/original buffer.
    for(u32 suffix_no = 0; suffix_no < n_suffixes; suffix_no++) {
      suffix_indexes[suffix_no] = sorted_suffix_indexes[suffix_no];
    }
    
    delete[] sorted_suffix_indexes;

    // Finally sort each bucket...
    // We'll do this recursively eventually...
    for(u32 c = 0; c < 256; c++) {
      u32 bucket_start = c == 0 ? 0 : radix_buf[c-1];
      u32 bucket_end = radix_buf[c];

      cpp_std_sort_suffixes(data, len, suffix_indexes + bucket_start, bucket_end - bucket_start);
    }

    return SUFFIX_SORT_OK;
  }

  int radix_sort(const u8* data, const u32 len, u32* SA) {
    // Count of prefixes with each initial byte.
    u32 radix_buf[RADIX_BUF_SIZE];

    init_sa_identity(SA, len);
    
    return radix_sort_level(data, len, SA, len, /*cp_len*/0, radix_buf);
  }

} // namespace SimpleSuffixSort

int simple_suffix_sort(const u8* data, const u32 len, u32* SA) {
  //return SimpleSuffixSort::cpp_std_sort(data, len, SA);
  return SimpleSuffixSort::radix_sort(data, len, SA);
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
  bool do_lcp = false;
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
  
  int rc = do_lcp ?
    simple_suffix_sort_with_lcp((const u8*)data, len, SA, LCP) :
    simple_suffix_sort((const u8*)data, len, SA);

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

    for(u32 rank = 0; rank < len; rank++) {
      u32 index = SA[rank];

      if(do_lcp) {
	printf("rank %8d index %8d [CP %8d]: %.*s\n", rank, index, LCP[rank], len-index, data+index);
      } else {
	printf("rank %8d index %8d: %.*s\n", rank, index, len-index, data+index);
      }
    }
  }
}

#endif //def SIMPLE_SUFFIX_SORT_MAIN
