#ifndef DECODE_H
#define DECODE_H

#include "types.h"

/*
 * Error codes returned from lz4_decode_block()
 */

/* Match offset exceeds current buffer offset. */
#define LZ4_DECODE_ERR_MATCH_OFFSET_TOO_LARGE 1

/**
 * Decompress a compressed lz4 block.
 * Optimised for little-endian platforms with cheap misaligned memory read/write.
 * @return size of decompressed data or -ve error code
 */
extern ssize_t lz4_decode_block(void* out_void, const size_t out_len, const void* in_void, const size_t in_len);

/**
 * Decompress a compressed lz4 block.
 * Default impl that works on all platforms.
 * @return size of decompressed data or -ve error code
 */
extern ssize_t lz4_decode_block_slow(void* out_void, const size_t out_len, const void* in_void, const size_t in_len);

#endif //ndef DECODE_H
