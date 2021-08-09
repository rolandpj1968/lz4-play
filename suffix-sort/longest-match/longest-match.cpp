#include <cstdio>
#include <utility>
#include <vector>

#include "longest-match.h"
#include "suffix-sort.h"

namespace LongestMatch {

  // Look for longest matches by scanning SA forwards.
  // Note that the match found here is not necessarily the best match - we also need to
  // scan backwards in case there is a better match.
  int scan_sa_forwards(const u32* SA, const u32* LCP, const u32 len, u32* LPM, u32* LML) {
    // Unmatched SA indexes - pair of (sa-index, common-prefix-length)
    // The stack is always ordered by data-index descending.
    std::vector<std::pair<u32, u32>> unmatched_stack;

    for(u32 sa_index = 0; sa_index < len; sa_index++) {
      // If this data index is lower than some of the unmatched indexes, then it's the
      // longest preceding match for those as-yet unmatched indexes.
      u32 data_index = SA[sa_index];

      while(!unmatched_stack.empty()) {
	u32 unmatched_sa_index = unmatched_stack.back().first;
	u32 unmatched_data_index = SA[unmatched_sa_index];

	// Since the unmatched stack is always ordered by data-index descending,
	// then as soon as we encounter a top-of-stack data-index preceding our
	// current data-index, we are done.
	if(unmatched_data_index < data_index) {
	  break;
	}

	// We have found the longest match.
	// TODO u32 unmatched_lcp = unmatched_stack.back().second;
	unmatched_stack.pop_back();

	LPM[unmatched_data_index] = data_index;
	LML[unmatched_data_index] = 0; // TODO
      }

      // Push the current sa_index onto the stack - we'll find its longest match as we scan
      // forwards in the SA.
      unmatched_stack.push_back(std::make_pair(sa_index, 0/*TODO*/));
    }

    // Once we've scanned the whole SA, then any SA indexes remaining on the stack have
    // no (non-empty) preceding match.
    while(!unmatched_stack.empty()) {
      u32 unmatched_sa_index = unmatched_stack.back().first;
      unmatched_stack.pop_back();
      
      u32 unmatched_data_index = SA[unmatched_sa_index];

      LPM[unmatched_data_index] = 0;
      LML[unmatched_data_index] = 0;
    }

    return LONGEST_MATCH_OK;
  }
  
  // Look for longest matches by scanning SA backwards.
  // It is assumed that we have already scanned forwards and LPM/LML contain the longest
  // matches found scanning forwards.
  // Here we replace the longest match from the forwards scan iff the longest match from
  // the backwards scan is longer.
  int scan_sa_backwards(const u32* SA, const u32* LCP, const u32 len, u32* LPM, u32* LML) {
    // Unmatched SA indexes - pair of (sa-index, common-prefix-length)
    // The stack is always ordered by data-index descending.
    std::vector<std::pair<u32, u32>> unmatched_stack;

    for(u32 sa_index_plus_one = len; sa_index_plus_one > 0; sa_index_plus_one--) {
      u32 sa_index = sa_index_plus_one - 1;
      // If this data index is lower than some of the unmatched indexes, then it's the
      // longest preceding match for those as-yet unmatched indexes.
      u32 data_index = SA[sa_index];

      while(!unmatched_stack.empty()) {
	u32 unmatched_sa_index = unmatched_stack.back().first;
	u32 unmatched_data_index = SA[unmatched_sa_index];

	// Since the unmatched stack is always ordered by data-index descending,
	// then as soon as we encounter a top-of-stack data-index preceding our
	// current data-index, we are done.
	if(unmatched_data_index < data_index) {
	  break;
	}

	// We have found the longest match.
	u32 unmatched_lml = 0; // TODO unmatched_stack.back().second;
	unmatched_stack.pop_back();

	// TODO - need LML populated correctly for this
	// TODO - we should also prefer closer matches if lml is equal
	if(LML[unmatched_data_index] < unmatched_lml) {
	  LPM[unmatched_data_index] = data_index;
	  LML[unmatched_data_index] = 0; // TODO
	}
      }

      // Push the current sa_index onto the stack - we'll find its longest match as we scan
      // forwards in the SA.
      unmatched_stack.push_back(std::make_pair(sa_index, 0/*TODO*/));
    }

    // Once we've scanned the whole SA, then any SA indexes remaining on the stack have
    // no (non-empty) preceding match.
    // Nothing to do here, since we don't have a longer match than the forwards scan match
    // (if there was one), or we (still) don't have a match at all.

    return LONGEST_MATCH_OK;
  }
  
} // namespace LongestMatch

extern int longest_matches(const u8* data, const u32 len, u32* LPM, u32* LML) {
  int rc = LONGEST_MATCH_OK;
  
  // Compute sorted suffix array SA and associated longest common prefix array LCP
  u32* SA = new u32[len];
  u32* LCP = new u32[len];

  int sa_rc = simple_suffix_sort_with_lcp(data, len, SA, LCP);

  if(sa_rc) {
    rc = sa_rc;
    goto out_delete;
  }

  // For each suffix, the longest preceding match can now be found by walking the SA
  // upwards and downwards until we find a suffix index preceding the suffix index.
  // The longest preceding match is then the best match found walking upwards or
  // downwards. We do this in O(N) by walking the entire SA forwards once, and then
  // walking the entire SA backwards once, keeping as-yet unmatched indexes on a stack
  // in both cases.

  rc = LongestMatch::scan_sa_forwards(SA, LCP, len, LPM, LML);
  if(rc) {
    goto out_delete;
  }

  // TODO rc = LongestMatch::scan_sa_backwards(SA, LCP, len, LPM, LML);

 out_delete:
  delete[] SA;
  delete[] LCP;

  return rc;
}

#ifdef LONGEST_MATCH_MAIN

// exit()
#include <cstdlib>
// strlen()
#include <cstring>

int main() {
  printf("Hallo RPJ\n");
  
  //const char *data = "banana";
  const char *data = "abracadabra banana abracadabra";
  const u32 len = strlen(data);

  u32* LPM = new u32[len];
  u32* LML = new u32[len];

  int rc = longest_matches((const u8*)data, len, LPM, LML);

  if(rc) {
    fprintf(stderr, "longest_matches failed with rc %d\n", rc);
    exit(1);
  }

  printf("data: %.*s\n\n", len, data);

  for(u32 i = 0; i < len; i++) {
    printf("%8d: %*.*s - match %8d length %8d: %*.*s\n", i, len, len-i, data+i, LPM[i], LML[i], len, (LPM[i] == 0 ? 0 : len-LPM[i]), data+LPM[i]); // TODO fix when LML is implemented
  }
}

#endif //def LONGEST_MATCH_MAIN
