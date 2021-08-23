//
// See "Dismantling DivSufSort" - https://arxiv.org/abs/1710.01896
//
// Original impl by Yuta Mori(?) Deconstruction by Johannes Fischer, Florian Kurpicz
//  who also added least common prefix (LCP) to the code (which doesn't work).
//
// Yuta Mori's code is currently pretty much the fastest way to do suffix sort
// and very space-efficient compared to other fast algorithms, mainly because the
// code uses the (eventual) suffix-array (SA) to maintain some interim data.
// Also because SA approaches are much more memory efficient than suffix tree
// approaches, of which there are many O(N) approaches. However, maintaining
// a suffix tree in memory is relatively memory expensive than a suffix array
// approach.
//
// I'm not yet convinced that Yuta Mori's code or algorithm are truly O(N),
// more of which below (but maybe my ignorance thus far).
//
// The algorithm:
//
// Each suffix (index) is characterised into "A" and "B" positions, where
// "A" is a "descending" suffix - i.e. an "A" suffix (or index) has first two
// alphabet characters, c0 and c1, where c0 > c1, and "B" suffixes have first
// two alphabet characters, c0 and c1, where c1 < c0.
//
// In this way A suffixes are immediately descending, and B suffixes are
// immediately ascending in alphabet order.
//
// We then extend this to the case where c0 == c1 - the suffix starting
// with identical characters  is deemed "A" or "B according to its immediate
// right suffix. In other words, where we have a run of character c in the
// string, every suffix in that run of c is given the A/B character of the
// suffix where the run ends.
//
// So where we have a run of character c0, c0, ... c1, we characterise every
// suffix in that run accoring to whether c0 > c1 ("A") or c0 < c1 ("B").
//
// By convention the end-of-string is less than any other character in the
// alphabet. For example, if the string ends in c0, c0, c0 then each of these
// suffix positions are deemed "A".
//
// For example, let's take alphabet 'a' < 'b' < 'c', and consider a long string
// of (just) those characters. Any suffix beginning "ab...", "ac..." or "bc..."
// is a "B" suffix. Any suffix beginning with "ba...", "ca..." or "cb..." is an
// "A" suffix.
//
// For runs of the same character, for example "aab..." or "aac...", the
// status of this prefix is provided by the final different character, so in
// these examples we have B (eventually ascending) suffixes. On the other a
// suffix starting with "bba..." is an A (eventually descending) suffix.
//
// It is trivial to observe that each suffix (index) of the string can be
// characterised as A or B in a single right-to-left pass of the string. The
// only (slight) difficulty is runs of the same alphabet character where we
// need to remember the A/B status of the end of the run. Since we are passing
// through the string right-to-left this is easy.
//
// Thus far we have characterised all suffixes (indexes) of the string as A or B
// according to their first two characters. It's worth noting that a "radix"/
// "counting" sort of the suffixes by their first character or first two
// characters give us an O(N) first-pass sort of the suffixes. We use both
// single-character prefix counting sort, and two-character-prefix counting sort
// in the remainder of the algorithm.
//
// Before we do that, we further characterise a subset of B suffixes (indexes)
// as B* indexes. A B* suffix (or index) ia a B suffix whose immediate right
// suffix is an A suffix.
//
// Some examples are in order. A suffix beginning "aba..." is a B* suffix,
// because it is "ab..." (B) and its immediate right suffix is "ba..." (A).
// On the other hand a suffix beginning "abc..." is not a B* suffix, because
// even tho it is a B suffix, its immediate right suffix is "bc...", also B.
//
// It's also worth considering runs of characters. For runs like "aaab...", all
// of the suffixes are B. However, only the last suffix in the run "ab..." can
// be B*, and only if (in this case) the next character is 'a' - in other words
// the suffix was 'aaba...' or aab[end-of-string].
//
// Now we have suffixes of the original string identified as A, B or B*
// suffixes.
//
// The real beauty of the algorithm is that we can infer, in O(N) by a single
// pass, firstly the ordering of (non-B*) B suffixes from B* suffixes, and then
// the ordering of A suffixes from (all including B*) B suffixes.
//
// First, however, we need to sort the B* suffixes. We first sort them by radix
// sort by first two characters - c0, c1. It should be noted that the Yuta Mori
// code identifies the (c0,c1) B* suffix index buckets as part of a single
// right-to-left pass through the string which is obviously possible if you
// consider it for a moment (exercise for the reader).
//
// Then for each (c0, c1) B* bucket - i.e. B* strings starting with c0, c1 we
// need to sort the B* strings in that bucket with total time O(N). Here I
// don't understand the algorithm, hence my skepticism about O(N).
//
// Let's take that for granted tho - we can sort the B* suffixes in total time
// O(N).
//
// We should also take a digression here into the (ordered) suffix array (SA)
// itself, and consider a radix sort of all suffixes on the first two
// characters - c0, c1.
//
// We know, from a single first-pass through the string exactly where B*
// suffixes will live in the (eventually) ordered suffix array. Let's consider
// the various cases of c0, c1 prefixes of the suffixes:
//
// 1. if c0 > c1 then there are no B or even B* prefixes. This (c0, c1) bucket
//    contains only A suffixes by definition of A
// 2. if c0 < c1 then this (c0, c1) bucket contains only B (and even B*)
//    prefixes. However, because all B* suffixes in this bucket are immediately
//    followed by an A (descending) suffix, then in this (c0, c1) bucket, all
//    of the B* suffixes precede the other B suffixes who are followed by an
//    ascending (greater) character.
// 3. if c0 == c1 then we have two classes:
//    3.1 This bucket contains A suffixes where after the run of c0 (==c1)
//        characters, there is c2 where c2 < c0 (or end-of-string), and
//    3.2 This bucket contains B suffixes where after the run of c0 (==c1)
//        characters, there is c2 where c2 > c0.
//    In both classes, 3.1 and 3.2 there are no B* suffixes, by definition.
//
// In summary, we know from counting (radix) sort on the first two characters
// of each (c0, c1) prefix of all suffixes where the eventual suffix array (SA)
// boundaries of of all B* suffixes are - they are at the start of every
// (c0, c1) bucket where c0 < c1, and we know exactly where the start of each
// (c0, c1) bucket is from a single (radix/counting) sort.
//
// Accordingly, we can sort all B* suffixes into their eventual full-SA
// position.
//
// Now, given sorted B* suffixes, we are able to infer firstly (non-B*) B
// suffix positions, firstly in a single pass, and then infer A suffix positions
// in a second single pass from all B suffix positions.
//
// Let's first take the inference of (non-B*) B suffixes from ordered B*
// suffixes:
//
// We run backwards through all B* (well actually all B) suffixes in sorted
// position. We know that all B* suffixes are ordered. For each B* (well
// actually B) suffix visited in reverse SA order (right-to-left) we look at the
// immediately left suffix. Let's say this immediately left suffix starts with
// c2, and the B* (actually B) suffix starts with (c0, c1). By definition, and
// the above, c0 < c1. As always there are two cases:
// 1. c2 > c0 - in this case the immediate leftmost suffix is an A suffix and
//    we will deal with A suffixes in the next pass - for now ignore.
// 2. c2 <= c0 - this is a B suffix and by definition not a B* suffix, because
//    c0 < c1. Since we are running right-to-left on the B* suffixes, this is
//    the greatest non-B* suffix we have found up til now. Hence we place the
//    suffix in its correct position in its (c2, c0) bucket, which is ahead of
//    us in our single right-to-left pass, and we will use it later to infer its
//    own immediate left-most suffix.
//
// We now have all B suffixes in their correct order in the SA array. It's
// important to not that we infer B positions right-to-left-wise from the
// starting set of B* suffixes. This right-to-left pass starting with B*
// suffixes in sorted order is best described as a pass inferring B suffix
// ordering from all B suffixes. Doing it right-to-left we only encounter
// non-B* B suffixes once they have already been inferred into their final SA
// position.
//
// Then we run forwards, inferring all A suffix positions from B suffixes. Note
// that the (eventual) suffix array (SA) starts with B suffixes, except for the
// special case of an initial string prefix consisting of repeats of the first
// alphabet character and ending in the [end-of-string].

// TODO - complete this ^^^ - I feel like writing code.

#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

#include <byteswap.h>

#include "suffix-sort.h"
#include "util.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

namespace BstarBA {

  // Count A prefixes in each c0-bucket and count B/B* prefixes in each (c0, c1)
  // bucket.
  // The B array is used for both B and B* counts per Yuta Mori divsufsort.
  // Also, log all B* indexes into the top of the given bstar_indexes buffer,
  //   which MUST be large enough to hold all the B* indexes - formally at least
  //   len/2.
  // Finally return the number of B* suffixes.
  u32 count_a_b_bstar(const u8* data, const u32 len, u32 A[256],
		       u32 B[256*256], u32* bstar_indexes, u32 bi_len) {
    memset(A, 0, 256*sizeof(A[0]));
    memset(B, 0, 256*256*sizeof(B[0]));

    // We log the B* indexes top-down into bstar_indexes, which means they
    // end up in-order, since we're running downwards through the data string.
    u32* bstar_cursor = &bstar_indexes[bi_len-1];

    // Last suffix is always A cos end-of-string is considered smaller than
    // any alphabet character
    bool was_B = false;
    u8 c0 = data[len-1];
    A[c0] += 1;

    // Run backwards through the data string.
    for(i32 index = len-2; index >= 0; index--) {
      u8 c1 = c0;
      c0 = data[index];

      bool is_B;

      if(c0 < c1) {
	// B
	is_B = true;
      } else if(c0 == c1) {
	// Same as previous
	is_B = was_B;
      } else /* c0 > c1 */ {
	// A
	is_B = false;
      }

      if(is_B) {
	if(was_B) {
	  // B but not not B*
	  B[c0*256 + c1] += 1;
	} else {
	  // B*
	  B[c1*256 + c0] += 1;
	  // Log the B* index
	  *bstar_cursor-- = index;
	}
      } else {
	A[c0] += 1;
      }

      was_B = is_B;
    }

    return &bstar_indexes[bi_len-1] - bstar_cursor;
  }

  // Param cond MUST be 0 or 1
  // return addr if cond, else NULL
  void* select_addr_if(void* addr, int cond) {
    // TODO should really use pointer-equiv integer size - this is 64-bit arch
    //   specific
    return (void *)(((i64)addr) & (-(i64)cond));
  }

  // Only one addr can be non-NULL, and rest MUST be NULL
  void* select_non_null_addr3(void* addr1, void* addr2, void* addr3) {
    return (void*)(((i64)addr1) | ((i64)addr2) | ((i64)addr3));
  }

  // Same as count_a_b_bstar(...)
  // Code attempts to be non-branching.
  // But it's slower than branching code :D
  u32 count_a_b_bstar_nobranch(const u8* data, const u32 len, u32 A[256],
				u32 B[256*256], u32* bstar_indexes, u32 bi_len) {
    memset(A, 0, 256*sizeof(A[0]));
    memset(B, 0, 256*256*sizeof(B[0]));

    // We log the B* indexes top-down into bstar_indexes, which means they
    // end up in-order, since we're running downwards through the data string.
    u32* bstar_cursor = &bstar_indexes[bi_len-1];

    // Last suffix is always A cos end-of-string is considered smaller than
    // any alphabet character
    int is_A = 1;
    u8 c0 = data[len-1];
    A[c0] += 1;

    // Run backwards through the data string.
    for(i32 index = len-2; index >= 0; index--) {
      int was_A = is_A;
      u8 c1 = c0;
      c0 = data[index];

      // Note we're doing bitwise arithmetic on lowest bit instead of
      // boolean branching logic.
      int is_eq = !!(c0 == c1);
      int is_gt = !!(c0 > c1);

      // Suffix is A if it's immediately descending, or immediately level but
      // eventually descending.
      is_A = is_gt | (is_eq & was_A);

      // Suffix is B* if it's not A but last suffix was A
      int is_Bstar = (!is_A) & was_A;

      // Suffix is B (and not B*) if it's neither A nor B*
      int is_B = (!is_A) & (!is_Bstar);

      //printf("          index %2d (%c, %c) was_A %d is_A %d is_B %d is_Bstar %d\n", index, c0, c1, was_A, is_A, is_B, is_Bstar);

      u32* addr = (u32*) select_non_null_addr3(select_addr_if(&A[c0], is_A),
					       select_addr_if(&B[c0*256 + c1], is_B),
					       select_addr_if(&B[c1*256 + c0], is_Bstar));
					       
      // Increment the bucket counter for this suffix type
      *addr += 1;

      // Log the B* index
      *bstar_cursor = index;
      bstar_cursor -= is_Bstar;
    }
    
    return &bstar_indexes[bi_len-1] - bstar_cursor;
  }

  // Sort the B* suffixes.
  // We need an O(N) algo here.
  // For now just straight c++ std::stable_sort().
  // Next step - radix sort over (c0, c1) buckets then sort each bucket.
  // bstar_buffer MUST be large enough to accommodate 2* n_bstar:
  //   On input, the (unsorted) B* indexes are at the end of the buffer;
  //   On output the sorted B* indexes will be at the start of the buffer.
  void sort_bstar(const u8* data, const u32 len, u32 A[256], u32 B[256*256],
		  u32* bstar_buffer, u32 bb_len, u32 n_bstar) {

    // Radix sort of the B* indexes on their first two character (c0, c1).

    // Transpose B* (c0, c1) bucket counts in B to bucket offsets.
    // The B* bucket counts are in the B[] array with (c0, c1) order inverted,
    //  so for all c0, c1 where c0 < c1 (only possible case for B*), the B[]
    //  index is B[c1*256 + c0]
    u32 offset = 0;
    for(u32 c0 = 0; c0 < 256; c0++) {
      for(u32 c1 = c0+1; c1 < 256; c1++) {
	u32 bucket_index = c1*256 + c0;
	u32 bucket_count = B[bucket_index];
	// if(c0 < 5) {
	//   printf("              offseting B* for 0x%02x, 0x%02x - offset is %u count %u\n", c0, c1, offset, bucket_count);
	// }
	B[bucket_index] = offset;
	offset += bucket_count;
      }
    }
    // printf("\n      final offset is %u expecting %u\n\n", offset, n_bstar);

    // for(u32 c0 = 0; c0 < 1; c0++) {
    //   for(u32 c1 = c0+1; c1 < 5; c1++) {
    // 	printf("                        offset 0x%02x 0x%02x is %u\n", c0, c1, B[c1*256 + c0]);
    //   }
    // }
    // printf("\n");

    // Radix sort the B* suffixes
    for(u32 bstar_i = 0; bstar_i < n_bstar; bstar_i++) {
      u32 bstar_index = bstar_buffer[bb_len-n_bstar+bstar_i];
      u32 c0 = data[bstar_index], c1 = data[bstar_index+1];
      // Ha - on little endian this is just a single non-aligned 2-byte mem read
      u32 B_index = c1*256 + c0;
      bstar_buffer[B[B_index]++] = bstar_index;
    }

    // printf("     after radix sort:\n");
    // for(u32 c0 = 0; c0 < 1; c0++) {
    //   for(u32 c1 = c0+1; c1 < 5; c1++) {
    // 	printf("                        offset 0x%02x 0x%02x is %u\n", c0, c1, B[c1*256 + c0]);
    //   }
    // }
    // printf("\n");
    
    // Then suffix-sort the B* indexes, bucket by bucket
    // TODO this is not O(N)
    u32 bucket_start_offset = 0;
    for(u32 c0 = 0; c0 < 256; c0++) {
      for(u32 c1 = c0+1; c1 < 256; c1++) {
	u32 bucket_index = c1*256 + c0;
	u32 bucket_end_offset = bucket_index == 255*256 + 254 ? n_bstar : B[bucket_index];
	u32 bucket_size = bucket_end_offset - bucket_start_offset;

	// if(c0 < 5) {
	//   printf("              sorting B* for 0x%02x, 0x%02x - bucket start is %u size %u\n", c0, c1, bucket_start_offset, bucket_size);
	// }

	if(bucket_size > 1) {
	  cpp_std_sort_suffixes((const u8*)data, len, bstar_buffer + bucket_start_offset, bucket_size);
	}
	bucket_start_offset = bucket_end_offset;
      }
    }
  }
  
  // Sort the B* suffixes.
  // We need an O(N) algo here.
  // For now just straight c++ std::stable_sort().
  // Next step - radix sort over (c0, c1) buckets then sort each bucket.
  // bstar_buffer MUST be large enough to accommodate 2* n_bstar:
  //   On input, the (unsorted) B* indexes are at the end of the buffer;
  //   On output the sorted B* indexes will be at the start of the buffer.
  void sort_bstar_dumb(const u8* data, const u32 len, u32 A[256], u32 B[256*256],
		  u32* bstar_buffer, u32 bb_len, u32 n_bstar) {
    // Copy the B* indexes from the end of bstar_buffer to the start - this will
    //   be replaced by a radix sort down-copy... TODO
    memcpy(bstar_buffer, bstar_buffer + bb_len - n_bstar, n_bstar * sizeof(u32));

    // Then suffix-sort the B* indexes
    cpp_std_sort_suffixes((const u8*)data, len, bstar_buffer, n_bstar);
  }

} // namespace BstarBA

#ifdef BSTAR_B_A_SUFFIX_SORT_MAIN

// std::string
#include <string>

// exit()
#include <cstdlib>

int main(int argc, char* argv[]) {
  printf("Hallo RPJ\n");

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

  u32* A = new u32[256];
  u32* B = new u32[256*256];

  u32* SA = new u32[len];
  
  auto t0 = Time::now();

  const int N_LOOPS = 10;

  u32 n_bstar = 0;
  for(int loop_no = 0; loop_no < N_LOOPS; loop_no++) {
    // Use SA as the temporary B* index buffer
    n_bstar = BstarBA::count_a_b_bstar((const u8*)data, len, A, B, SA, len);
    //n_bstar = BstarBA::count_a_b_bstar_nobranch((const u8*)data, len, A, B, SA, len);

    // Sort the B* indexes
    BstarBA::sort_bstar((const u8*)data, len, A, B, SA, len, n_bstar);
  }

  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Count A/B/B* of data string length %u bytes in %7.3lfms\n", len, secs1/N_LOOPS*1000.0);

  // Some stats
  u32 nA = 0, maxA = 0, maxABucket = 0;
  u32 nB = 0, maxB = 0, maxBBucket = 0;
  u32 nBstar = 0, maxBstar = 0, maxBstarBucket = 0;

  for(u32 c0 = 0; c0 < 256; c0++) {
    nA += A[c0];
    if(A[c0] > maxA) {
      maxA = A[c0];
      maxABucket = c0;
    }
    for(u32 c1 = 0; c1 < 256; c1++) {
      u32 bucket = c0*256 + c1;
      if(c0 <= c1) {
	nB += B[bucket];
	if(B[bucket] > maxB) {
	  maxB = B[bucket];
	  maxBBucket = bucket;
	}
      } else {
	nBstar += B[bucket];
	if(B[bucket] > maxBstar) {
	  maxBstar = B[bucket];
	  maxBstarBucket = c1*256 + c0;
	}
      }
    }
  }

  //printf("A %u maxA %u maxA-bucket 0x%04x\n", nA, maxA, maxABucket);
  printf("A %u maxA %u maxA-bucket 0x%02x\n", nA, maxA, maxABucket);
  printf("B %u maxB %u maxB-bucket 0x%04x\n", nB, maxB, maxBBucket);
  printf("Bstar %u maxBstar %u maxBstar-bucket 0x%04x\n", nBstar, maxBstar, maxBstarBucket);
  printf("total %u expecting %u\n", nA + nB + nBstar, len);
}

#endif //def BSTAR_B_A_SUFFIX_SORT_MAIN
