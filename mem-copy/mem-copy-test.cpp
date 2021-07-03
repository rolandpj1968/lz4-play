#include <chrono>
#include <cstdio>
#include <cstring>

#include "mem-copy.h"
#include "types.h"

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

const size_t MiB = 1 << 20;

const double ms_per_s = 1000.0;

static u8 src[2*MiB + 7];
static u8 dst[2*MiB + 7];

static const size_t copy_len = 2*MiB;

static double time_mem_copy_ms(void* dst, const void* src, size_t len, unsigned n_iters) {
  auto t0 = Time::now();

  for(unsigned i = 0; i < n_iters; i++) {
    mem_copy_u64_u64_raw(dst, src, copy_len);
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  return secs1 * ms_per_s;
}

static double time_memcpy_ms(void* dst, const void* src, size_t len, unsigned n_iters) {
  auto t0 = Time::now();

  for(unsigned i = 0; i < n_iters; i++) {
    memcpy_it(dst, src, copy_len);
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  return secs1 * ms_per_s;
}

int main(int argc, char* argv[]) {
  printf("Hallo RPJ\n\n");

  auto t0 = Time::now();

  const unsigned n_loops = 4;
  const unsigned n_iters = 1000;

  for(unsigned i = 0; i < n_loops; i++ ) {
  
    for(size_t src_offset = 0; src_offset < 8; src_offset++) {
      for(size_t dst_offset = 0; dst_offset < 8; dst_offset++) {
	double ms;
	double mib_per_s;
	
	ms = time_mem_copy_ms(dst + dst_offset, src + src_offset, copy_len, n_iters);
	
	mib_per_s = copy_len*n_iters/MiB / (ms/ms_per_s);
	printf("mem_copy: copied %zu bytes to %p from %p repeated %u times in %7.3lfms - %10.3lfMiB/s\n", copy_len, dst + dst_offset, src + src_offset, n_iters, ms, mib_per_s);
	
	ms = time_memcpy_ms(dst + dst_offset, src + src_offset, copy_len, n_iters);
	
	mib_per_s = copy_len*n_iters/MiB / (ms/ms_per_s);
	printf("memcpy:   copied %zu bytes to %p from %p repeated %u times in %7.3lfms - %10.3lfMiB/s\n\n", copy_len, dst + dst_offset, src + src_offset, n_iters, ms, mib_per_s);
      }
    }
    
    printf("\n");
  }
  
  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();

  printf("Done in %7.3lfms\n", secs1*1000.0);
}
