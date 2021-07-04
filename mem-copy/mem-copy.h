#ifndef MEM_COPY_H
#define MEM_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void libc_memcpy(void* dst, const void* src, size_t len);

void mem_copy_u64_simple(void* dst, const void* src, size_t len);
void mem_copy_u64(void* dst, const void* src, size_t len);
void mem_copy_u64_restrict(void* dst, const void* src, size_t len);

void mem_copy_u64_u64_simple(void* dst, const void* src, size_t len);
void mem_copy_u64_u64(void* dst, const void* src, size_t len);
void mem_copy_u64__u64_restrict(void* dst, const void* src, size_t len);
	
#ifdef __cplusplus
}
#endif

#endif //def MEM_COPY_H
