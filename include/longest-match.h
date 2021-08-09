#ifndef LONGEST_MATCH_H
#define LONGEST_MATCH_H

#include <types.h>

#define LONGEST_MATCH_OK 0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * For each (suffix) index in the input string, find the longest preceding matching
 * (suffix) index output in LPM, and the length of that match output in LML.
 *
 * Where there is no preceding match, LML[i] is 0 and LPM[i] is irrelevant.
 * 
 * The longest match algorithm uses suffix sort (SA) and longest common prefix (LCP)
 * computation so will be complexity bound by the implementation of SA+LCP calculation.
 *
 * However, calculation of the longest preceding match is O(N) in the data length
 * once we have SA+LCP. There are O(N) algorithms for computing SA+LCP so with good
 * SA+LCP implementation the full longest-preceding-match algorithm is O(N).
 *
 * For things like lz4 we really want the longest preceding matching string within
 * a smaller window (e.g. 64KiB in lz4) - TODO.
 *
 * @return rc
 */
extern int longest_matches(const u8* data, const u32 len, u32* LPM, u32* LML);

#ifdef __cplusplus
}
#endif
  
#endif //def LONGEST_MATCH_H
