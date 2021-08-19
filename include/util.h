#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#ifdef __cplusplus

#include <string>

namespace Util {
  std::string slurp(const std::string& filepath);
} // namespace Util

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
  
#ifdef __cplusplus
}
#endif

#endif //def UTIL_H
