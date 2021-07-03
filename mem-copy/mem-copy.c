#include <string.h>


#include "types.h"

// It's really hard to get gcc to avoid alignment assumptions and
// generate MMX-optimised copy code, hence jumping through hoops
// to prevent this.

static inline u64 read_u64(const u8* p) {
  u64 v = 0;

  v |= ((u64)p[0]);
  v |= ((u64)p[1]) << (1*8);
  v |= ((u64)p[2]) << (2*8);
  v |= ((u64)p[3]) << (3*8);
  v |= ((u64)p[4]) << (4*8);
  v |= ((u64)p[5]) << (5*8);
  v |= ((u64)p[6]) << (6*8);
  v |= ((u64)p[7]) << (7*8);
  
  return v;
}

static inline u64 write_u64(u8* p, u64 v) {
  p[0] = (u8) (v);
  p[1] = (u8) (v >> (1*8));
  p[2] = (u8) (v >> (2*8));
  p[3] = (u8) (v >> (3*8));
  p[4] = (u8) (v >> (4*8));
  p[5] = (u8) (v >> (5*8));
  p[6] = (u8) (v >> (6*8));
  p[7] = (u8) (v >> (7*8));
}

void memcpy_it(void* dst, const void* src, size_t len) {
  memcpy(dst, src, len);
}

// Copy u64 at a time - simplest code
// gcc -O3 assumes aligned u64* and optimises to MMX instructions
//   which SIGSEGV's on non-aligned addresses.
// However, gcc -O1 does very basic optimisation keeping vars in
//   registers and this is eventually ~2x slower than memcpy().
// It's also ~7x faster at -O1 than mem_copy_u64 at -O3 (!!!)
void mem_copy_u64_raw(void* dst, const void* src, size_t len) {
  u64* dst_u64 = (u64*)dst;
  const u64* src_u64 = (const u64*)src;
  
  size_t len_u64 = len/sizeof(u64);
  const u64* src_u64_limit = src_u64 + len_u64;

  while(src_u64 < src_u64_limit) {
    *dst_u64++ = *src_u64++;
  }
}

// Copy u64 at a time
void mem_copy_u64(void* dst, const void* src, size_t len) {
  u8* dst_u8 = (u8*)dst;
  const u8* src_u8 = (const u8*)src;

  const u8* src_u8_limit = src_u8 + (len & ~0x7);

  while(src_u8 < src_u8_limit) {
    u64 v = read_u64(src_u8);
    src_u8 += sizeof(u64);
    
    write_u64(dst_u8, v);
    dst_u8 += sizeof(u64);
  }

  // TODO extra bytes at end
}

// Copy u64 at a time with restrict keyword on src, dst
void mem_copy_u64_restrict(void* dst, void* src, size_t len) {
  u8* restrict dst_u8 = (u8*)dst;
  const u8* restrict src_u8 = (const u8*)src;

  const u8* src_u8_limit = src_u8 + (len & ~0x7);

  while(src_u8 < src_u8_limit) {
    u64 v = read_u64(src_u8);
    src_u8 += sizeof(u64);
    
    write_u64(dst_u8, v);
    dst_u8 += sizeof(u64);
  }

  // TODO extra bytes at end
}

// Copy 2 x u64 at a time - simplest code
// gcc -O3 assumes aligned u64* and optimises to MMX instructions
//   which SIGSEGV's on non-aligned addresses.
// However, gcc -O1 does very basic optimisation other than keeping vars in
//   registers and this is eventually only ~20% slower than memcpy() !!!
// Winner!
void mem_copy_u64_u64_raw(void* dst, const void* src, size_t len) {
  u64* dst_u64 = (u64*)dst;
  const u64* src_u64 = (const u64*)src;
  
  size_t len_u64 = len/(2*sizeof(u64));
  const u64* src_u64_limit = src_u64 + len_u64;

  while(src_u64 < src_u64_limit) {
    *dst_u64++ = *src_u64++;
    *dst_u64++ = *src_u64++;
  }
}

// Copy 2 x u64 at a time
void mem_copy_u64_u64(void* dst, void* src, size_t len);

// Copy 2 x u64 at a time with restrict keyword on src, dst
void mem_copy_u64__u64_restrict(void* dst, void* src, size_t len);
