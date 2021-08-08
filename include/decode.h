#ifndef DECODE_H
#define DECODE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Error codes returned from lz4_decode_block()
 */

/* Match offset exceeds current output length. */
#define LZ4_DECODE_ERR_MATCH_OFFSET_TOO_LARGE 1
/* Insufficient space in output buffer. */
#define LZ4_DECODE_ERR_OUTPUT_OVERFLOW 2
/* Input buffer overrun in the middle of a sequence. */
#define LZ4_DECODE_ERR_INPUT_OVERFLOW 2

/**
 * Decompress a compressed lz4 block.
 * Optimised for little-endian platforms with cheap misaligned memory read/write.
 * @return size of decompressed data or -ve error code
 */
extern ssize_t lz4_decode_block_fast(void* out_void, const size_t out_len, const void* in_void, const size_t in_len);

/**
 * Decompress a compressed lz4 block.
 * Default impl that works on all platforms.
 * @return size of decompressed data or -ve error code
 */
extern ssize_t lz4_decode_block_default(void* out_void, const size_t out_len, const void* in_void, const size_t in_len);

#ifdef __cplusplus
}
#endif
  
#endif //ndef DECODE_H
