#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef int32_t i32;
  
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef void mem_copy_fn(void* dst, const void* src, size_t len);
	
#ifdef __cplusplus
}
#endif

#endif //def TYPES_H
