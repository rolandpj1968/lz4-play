#include <chrono>
#include <cstdio>
#include <cstring>

#include "mem-copy.h"
#include "types.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

static u8 src[1 << 21 + 7];
static u8 dst[1 << 21 + 7];

static const size_t copy_len = 1 << 21;

static double time_mem_copy_ms(void* dst, const void* src, size_t len, unsigned n_iters) {
  auto t0 = Time::now();

  for(unsigned i = 0; i < n_iters; i++) {
    mem_copy_u64_u64_raw(dst, src, copy_len);
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  return secs1 * 1000.0/*ms per s*/;
}

static double time_memcpy_ms(void* dst, const void* src, size_t len, unsigned n_iters) {
  auto t0 = Time::now();

  for(unsigned i = 0; i < n_iters; i++) {
    memcpy_it(dst, src, copy_len);
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  return secs1 * 1000.0/*ms per s*/;
}

int main(int argc, char* argv[]) {
  printf("Hallo RPJ\n\n");

  auto t0 = Time::now();

  const unsigned n_loops = 4;
  const unsigned n_iters = 1000;

  for(unsigned i = 0; i < n_loops; i++ ) {
  
    for(size_t src_offset = 0; src_offset < 8; src_offset++) {
      double ms;

      ms = time_mem_copy_ms(dst, src + src_offset, copy_len, n_iters);
      printf("mem_copy: copied %zu bytes to %p from %p repeated %u times in %7.3lfms\n", copy_len, dst, src + src_offset, n_iters, ms);
      ms = time_memcpy_ms(dst, src + src_offset, copy_len, n_iters);
      printf("memcpy:   copied %zu bytes to %p from %p repeated %u times in %7.3lfms\n", copy_len, dst, src + src_offset, n_iters, ms);
    }
    
    printf("\n");
    
    for(size_t dst_offset = 0; dst_offset < 8; dst_offset++) {
      mem_copy_u64(dst + dst_offset, src, copy_len);
      printf("Copied %zu bytes to %p from %p\n", copy_len, dst + dst_offset, src);
    }
    
    printf("\n");
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Done in %7.3lfms\n", secs1*1000.0);
}
