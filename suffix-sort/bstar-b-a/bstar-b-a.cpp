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

#include "suffix-sort.h"

namespace BstarBA {

  enum SuffixType { SuffixType_A, SuffixType_B, SuffixType_Bstar };

  // Count number A, B and B* prefixes in each (c0, c1) bucket. The only suffix
  // excluded from this count is the very last single-character suffix.
  // TODO can use single array for B and Bstar like divsufsort.
  // TODO divsufsort only counts A for single-char buckets.
  void count_a_b_bstart_per_bucket(const u8* data, const u32 len, u32 A[256*256],
				   u32 B[256*256], u32 Bstar[256*256]) {
    memset(A, 0, 256*256*sizeof(A[0]));
    memset(B, 0, 256*256*sizeof(B[0]));
    memset(Bstar, 0, 256*256*sizeof(Bstar[0]));

    u32* counts[] = { A, B, Bstar };

    // The single-character last suffix is considered 'A' since end-of-string is
    // considered to precede all alphabet characters.
    SuffixType last_suffix_type = SuffixType_A;

    for(u32 index_plus_one = len - 1; index_plus_one > 0; index_plus_one--) {
      u32 index = index_plus_one - 1;
      u8 c0 = data[index];
      u8 c1 = data[index+1];

      SuffixType suffix_type;
      if(c0 > c1) {
	suffix_type = SuffixType_A;
      } else if(c0 == c1) {
	suffix_type = last_suffix_type;
      } else /*c0 < c1*/ {
	if(last_suffix_type == SuffixType_A) {
	  suffix_type = SuffixType_Bstar;
	} else {
	  suffix_type = SuffixType_B;
	}
      }

      counts[suffix_type][c0*256 + c1]++;

      last_suffix_type = suffix_type;
    }
  }
} // namespace BstarBA



