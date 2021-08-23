#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#ifdef __cplusplus

#include <string>

extern "C" {
#endif

static inline u16 u16_at_offset(const void* buf, size_t offset) {
  // Use misaligned memory access for x86
  u8* p = ((u8*)buf) + (size_t)offset;
  return *(u16*)p;
}
  
static inline u32 u32_at_offset(const void* buf, size_t offset) {
  // Use misaligned memory access for x86
  u8* p = ((u8*)buf) + (size_t)offset;
  return *(u32*)p;
}
  
static inline u64 u64_at_offset(const void* buf, size_t offset) {
  // Use misaligned memory access for x86
  u8* p = ((u8*)buf) + (size_t)offset;
  return *(u64*)p;
}

bool suffix_less(const u8* data, const u32 len, const u32 i1, const u32 i2);

extern int n_cpp_std_sorts;  
void cpp_std_sort_suffixes(const u8* data, const u32 len, u32* suffix_indexes, u32 n_suffixes);
  
#ifdef __cplusplus
namespace Util {
  std::string slurp(const std::string& filepath);

  struct SuffixLess {
    const u8* data;
    const u32 len;

    SuffixLess(const u8* data, const u32 len)
      : data(data), len(len) {}

    inline bool operator()(const u32 i1, const u32 i2) {
      return suffix_less(data, len, i1, i2);
    }
  }; // struct SuffixLess

} // namespace Util

} //extern "C"
#endif //def __cplusplus

#endif //def UTIL_H
