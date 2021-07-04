#include <chrono>
#include <cstdio>
#include <cstring>

#include "mem-copy.h"
#include "types.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

const size_t MiB = 1 << 20;
const size_t KiB = 1 << 10;

const double ms_per_s = 1000.0;

//static const size_t copy_len = 2*MiB;
//static const size_t copy_len = 4*KiB;
static const size_t copy_len = 128;
//static const size_t copy_len = 32;
//static const size_t copy_len = 27;
//static const size_t copy_len = 24;
//static const size_t copy_len = 16;
//static const size_t copy_len = 8;
const unsigned n_loops = 4;
const unsigned n_iters = 2*MiB/copy_len * 1000;

// Add 16 for overrun and 7 for alignment offset
static u8 src[copy_len + 16 + 7];
static u8 dst[copy_len + 16 + 7];

static void time_mem_copy_fn_ms(const char* desc, mem_copy_fn mem_copy, void* dst, const void* src, size_t len, unsigned n_iters) {
  auto t0 = Time::now();

  for(unsigned i = 0; i < n_iters; i++) {
    mem_copy(dst, src, copy_len);
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  double ms = secs1 * ms_per_s;
  double mib_per_s = copy_len*n_iters/MiB / secs1;
  
  printf("%-24s copied %zu bytes to %p from %p repeated %u times in %9.3lfms - %10.3lfMiB/s\n", desc, copy_len, dst, src, n_iters, ms, mib_per_s);
}

int main(int argc, char* argv[]) {
  printf("Hallo RPJ\n\n");

  auto t0 = Time::now();

  for(unsigned i = 0; i < n_loops; i++ ) {
  
    for(size_t src_offset = 0; src_offset < 8; src_offset++) {
      for(size_t dst_offset = 0; dst_offset < 8; dst_offset++) {
	
	time_mem_copy_fn_ms("mem_copy_u64_simple", mem_copy_u64_simple, dst + dst_offset, src + src_offset, copy_len, n_iters);

	time_mem_copy_fn_ms("mem_copy_u64_u64_simple", mem_copy_u64_u64_simple, dst + dst_offset, src + src_offset, copy_len, n_iters);

	time_mem_copy_fn_ms("libc_memcpy", libc_memcpy, dst + dst_offset, src + src_offset, copy_len, n_iters);

	printf("\n");
      }
    }
    
    printf("\n");
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Done in %7.3lfms\n", secs1*1000.0);
}
