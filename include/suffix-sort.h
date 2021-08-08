#ifndef SUFFIX_SORT_H
#define SUFFIX_SORT_H

#include <types.h>

#define SUFFIX_SORT_OK 0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Populate the suffix array SA with the suffix sort of data.
 * @return rc
 */
extern int simple_suffix_sort(const u8* data, const u32 len, u32* SA);
  
#ifdef __cplusplus
}
#endif
  
#endif //def SUFFIX_SORT_H
