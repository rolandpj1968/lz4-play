#ifndef SUFFIX_SORT_H
#define SUFFIX_SORT_H

#include <types.h>

#define SUFFIX_SORT_OK 0
#define LCP_OK 0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Populate the suffix array SA with the suffix sort of data.
 * Naive N^2logN algorithm.
 * @return rc
 */
extern int simple_suffix_sort(const u8* data, const u32 len, u32* SA);

/**
 * Populate the longest common prefix array LCP from the index array SA.
 * Naive N^2 algorithm.
 * @return rc
 */
extern int simple_lcp(const u8* data, const u32 len, const u32* SA, u32* LCP);

/**
 * Populate the suffix array SA with the suffix sort of data.
 * Naive N^2logN + N^2 algorithms.
 * @return rc
 */
extern int simple_suffix_sort_with_lcp(const u8* data, const u32 len, u32* SA, u32* LCP);

#ifdef __cplusplus
}
#endif
  
#endif //def SUFFIX_SORT_H
