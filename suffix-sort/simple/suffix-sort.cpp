#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#include <byteswap.h>

#include "suffix-sort.h"
#include "util.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

namespace SimpleSuffixSort {

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
  static void init_sa_identity(u32* SA, const u32 len) {
    for(u32 i = 0; i < len; i++) {
      SA[i] = i;
    }
  }

  int cpp_std_sort(const u8* data, const u32 len, u32* SA) {
    init_sa_identity(SA, len);

    cpp_std_sort_suffixes(data, len, SA, len);

    return SUFFIX_SORT_OK;
  }

  static const size_t RADIX_BUF_SIZE = 256 + 1; // u8 range plus one extra
  static const u32 MAX_RADIX_SORT_LEVEL = 2;
  static const u32 MIN_RADIX_SORT_SIZE = 1;

  int n_radix_sorts = 0;

  // Stable sort a set of suffixes with common prefix in-place.
  // It's not really in-place but output is in-place.
  // cp_len is known common-prefix length of all suffixes, which is the same as
  //   the recursive radix sort level - we will look at the next char of each
  //   suffix for this radix level.
  // sorting_buf must be at least as big as suffix_indexes.
  int radix_sort_level(const u8* data, const u32 len, u32* suffix_indexes,
		       u32 n_suffixes, u32 cp_len,
		       u32* sorting_buf) {

    if(n_suffixes <= 1) {
      // Nothing to do - it's already trivially sorted...
      return SUFFIX_SORT_OK;
    }

    n_radix_sorts++;

    // Watch out for stack overflow.
    u32 radix_buf[RADIX_BUF_SIZE];

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
      //printf("                             radix 0x%02x count %u\n", c, radix_buf[c+1]);
      offset += radix_buf[c+1];
      radix_buf[c+1] = offset;
    }

    // Place each suffix into its radix bucket using the sorting_buf as
    //   temporary working space.
    u32* sorted_suffix_indexes = sorting_buf;

    for(u32 suffix_no = 0; suffix_no < n_suffixes; suffix_no++) {
      u32 suffix_index = suffix_indexes[suffix_no];
      u8 c = data[suffix_index + cp_len];

      u32 sorted_suffix_offset = radix_buf[c]++;
      sorted_suffix_indexes[sorted_suffix_offset] = suffix_index;
    }

    // Copy the buckets back to the original array.
    // We could avoid this copy with alternating temp/original buffer.
    for(u32 suffix_no = 0; suffix_no < n_suffixes; suffix_no++) {
      suffix_indexes[suffix_no] = sorted_suffix_indexes[suffix_no];
    }
    
    // Recursively sort each bucket...
    for(u32 c = 0; c < 256; c++) {
      u32 bucket_start = c == 0 ? 0 : radix_buf[c-1];
      u32 bucket_end = radix_buf[c];

      u32* bucket_suffix_indexes = suffix_indexes + bucket_start;
      u32 n_bucket_suffixes = bucket_end - bucket_start;

      if(n_bucket_suffixes > 1) {
	// Use radix sort recursively up til MAX_RADIX_SORT_LEVEL
	if(cp_len + 1 < MAX_RADIX_SORT_LEVEL && MIN_RADIX_SORT_SIZE <= n_bucket_suffixes) {
	  radix_sort_level(data, len, bucket_suffix_indexes, n_bucket_suffixes,
			   cp_len + 1, sorting_buf);
	} else {
	  cpp_std_sort_suffixes(data, len, bucket_suffix_indexes, n_bucket_suffixes);
	}
      }
    }

    return SUFFIX_SORT_OK;
  }

  int radix_sort(const u8* data, const u32 len, u32* SA) {
    // Temporary space for sorting
    u32* sorting_buf = new u32[len];
    
    init_sa_identity(SA, len);
    
    int rc = radix_sort_level(data, len, SA, len, /*cp_len*/0, sorting_buf);

    delete[] sorting_buf;

    return rc;
  }

  int simple_suffix_sort(const u8* data, const u32 len, u32* SA) {
    if(MAX_RADIX_SORT_LEVEL == 0) {
      return SimpleSuffixSort::cpp_std_sort(data, len, SA);
    } else {
      return SimpleSuffixSort::radix_sort(data, len, SA);
    }
  }

} // namespace SimpleSuffixSort

int simple_suffix_sort(const u8* data, const u32 len, u32* SA) {
  return SimpleSuffixSort::simple_suffix_sort(data, len, SA);
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

  u8 u8s[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  printf("u64 at offset 0 is 0x%016lx, u64 at offset 1 is 0x%016lx\n", u64_at_offset(u8s, 0), u64_at_offset(u8s, 1));

  std::string data_string;
  bool do_lcp = false;
  bool show_suffixes = false;

  if(argc <= 1) {
    // data = "banana";
    data_string = "abracadabra banana abracadabra";
    show_suffixes = true;
  } else {
    std::string filename = argv[1];
    data_string = Util::slurp(filename);
  }
  
  const char* data = data_string.c_str();;
  u32 len = data_string.length();

  printf("Using data string of length %u bytes\n", len);

  u32* SA = new u32[len];
  u32* LCP = new u32[len];

  auto t0 = Time::now();

  const int N_LOOPS = 10;

  int rc = SUFFIX_SORT_OK;
  for(int loop_no = 0; loop_no < N_LOOPS; loop_no++) {
    rc = do_lcp ?
      simple_suffix_sort_with_lcp((const u8*)data, len, SA, LCP) :
      simple_suffix_sort((const u8*)data, len, SA);
  }

  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Suffix array (SA) sort %sof data string length %u bytes in %7.3lfms\n", (do_lcp ? "with least-common-prefix (LCP) " : ""), len, secs1/N_LOOPS*1000.0);

  printf("          including %d std::sorts and %d radix sorts\n", n_cpp_std_sorts, SimpleSuffixSort::n_radix_sorts);
  
  if(rc) {
    fprintf(stderr, "simple_suffix_sort_with_lcp failed with rc %d\n", rc);
    exit(1);
  }

  // Check suffix array
  for(u32 i = 1; i < len; i++) {
    u32 index1 = SA[i-1], index2 = SA[i];
    if(!suffix_less((const u8*)data, len, index1, index2)) {
      printf("SA[%u] = %u starting 0x%02x is not less than SA[%u] = %u starting 0x%02x\n", i-1, index1, data[index1], i, index2, data[index2]);
      u64 prefix1 = u64_at_offset(data, index1), prefix2 = u64_at_offset(data, index2);
      printf("  u64 at %u is 0x%016lx, u64 at %u is 0x%016lx\n", index1, prefix1, index2, prefix2);
      
      exit(1);
    }
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
