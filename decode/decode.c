#include <stdio.h>
// memcpy, memmove
#include <string.h>

#include "decode.h"
#include "types.h"

// Speculatively read and write up to 16 bytes of lits 
#define LITS_LOOKAHEAD (16)

// Speculatively read ahead 1 byte of long lit
#define LONG_LIT_LOOKAHEAD (1)

// Speculatively read and write up to 16 bytes of match
#define MATCH_LOOKAHEAD (16)

// sizeof lit-len/match-len token in lz4 sequence
#define LITS_LEN_MATCH_LEN_TOKEN_SIZE (1)

// bit width of LITS_LEN in token
#define LITS_LEN_BITS (4)
// distinguished value for "long lits"
#define LONG_LITS_LEN (15)
// distinguished length extension value for "long lits"
#define LITS_LEN_EXTENSION_EXTRA (255)

// Speculatively read ahead 1 byte of long lit
#define LONG_MATCH_LOOKAHEAD (1)

// bit-mask of MATCH_LEN in token
#define MATCH_LEN_MASK (0xf)
// MATCH_LEN offset - minimum match len is 4
#define MATCH_LEN_MIN (4)
// distinguished value for "long lits"
#define LONG_MATCH_LEN (15 + MATCH_LEN_MIN)
// distinguished length extension value for "long match"
#define MATCH_LEN_EXTENSION_EXTRA (255)

// sizeof match offset in lz4 sequence
#define MATCH_OFFSET_LEN (2)

// Total speculative look-ahead on input buffer
#define IN_LOOKAHEAD (LITS_LEN_MATCH_LEN_TOKEN_SIZE + LITS_LOOKAHEAD + LONG_LIT_LOOKAHEAD + MATCH_OFFSET_LEN + LONG_MATCH_LOOKAHEAD)

// Total speculative look-ahead on output buffer
#define OUT_LOOKAHEAD (LITS_LOOKAHEAD + MATCH_LOOKAHEAD)


// TODO
#define xCONFIG_USE_LIKELY
#define register

#ifdef CONFIG_USE_LIKELY
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif //def CONFIG_USE_LIKELY

static inline size_t token_to_lits_len(u8 token) {
  return token >> LITS_LEN_BITS;
}

static inline size_t token_to_match_len(u8 token) {
  return (token & MATCH_LEN_MASK) + MATCH_LEN_MIN;
}

// No limitations
// @return decoded data length or -ve error val
ssize_t lz4_decode_block_default(void* out_void, const size_t out_len, const void* in_void, const size_t in_len) {
  u8* out_start = (u8*)out_void;
  u8* out = (u8*)out_void;
  u8* out_limit = out + out_len;
  
  //const u8* in_start = (u8*)in_void;
  const u8* in = (const u8*)in_void;
  const u8* in_limit = in + in_len;

  while(in < in_limit && out < out_limit) {
    //printf("                         sequence at in %lu out %lu\n", in - in_start, out - out_start);
    // Parse literals- and match length token
    u8 lits_len_match_len_token = *in++;
    size_t lits_len = token_to_lits_len(lits_len_match_len_token);
    size_t match_len = token_to_match_len(lits_len_match_len_token);

    //printf("                           token-lits-len %lu, token-match-len %lu\n", lits_len, match_len);
    
    // Handle literals length extension
    if(lits_len == LONG_LITS_LEN) {
      u8 lits_len_extension;
      do {
	if(!(in < in_limit)) {
	  return -LZ4_DECODE_ERR_INPUT_OVERFLOW;
	}
	lits_len_extension = *in++;
	lits_len += lits_len_extension;
      } while(lits_len_extension == LITS_LEN_EXTENSION_EXTRA);
    }

    //printf("                                       lits-len %lu\n", lits_len);
    
    // Handle literals
    if(!(in + lits_len <= in_limit)) {
      return -LZ4_DECODE_ERR_INPUT_OVERFLOW;
    }
    if(!(out + lits_len <= out_limit)) {
      return -LZ4_DECODE_ERR_OUTPUT_OVERFLOW;
    }

    memcpy(out, in, lits_len);
    
    out += lits_len;
    in += lits_len;

    // The last sequence in a block does not have a match - handle this special case.
    if(in < in_limit) {
      // Handle match offset
      if(!(in + MATCH_OFFSET_LEN <= in_limit)) {
	return -LZ4_DECODE_ERR_INPUT_OVERFLOW;
      }
      
      size_t match_offset = (size_t)in[0] + (((size_t)in[1]) << 8);

      in += MATCH_OFFSET_LEN;
      
      //printf("                                       match-offset %lu\n", match_offset);
      
      // Do comparison this way to avoid underflow of out_start.
      if(out - out_start < match_offset) {
	return -LZ4_DECODE_ERR_MATCH_OFFSET_TOO_LARGE;
      }

      // Handle match length extension
      if(match_len == LONG_MATCH_LEN) {
	u8 match_len_extension;
	do {
	  if(!(in < in_limit)) {
	    return -LZ4_DECODE_ERR_INPUT_OVERFLOW;
	  }
	  match_len_extension = *in++;
	  match_len += match_len_extension;
	} while(match_len_extension == MATCH_LEN_EXTENSION_EXTRA);
      }

      //printf("                                       match-len %lu\n", match_len);
    
      // Handle match bytes
      // TODO - this can actually overflow top-end of address range :D
      if(!(out + match_len <= out_limit)) {
	return -LZ4_DECODE_ERR_OUTPUT_OVERFLOW;
      }

      u8* match = out - match_offset;
      // Input can overlap output so use memmove()
      memmove(out, match, match_len);
      
      out += match_len;
    }
  }

  if(in < in_limit) {
    return -LZ4_DECODE_ERR_OUTPUT_OVERFLOW;
  }

  return out - out_start;
}

// Limitations:
// Assumes non-aligned memory accesses work with primitive C integer types - undefined officially
// Assumes little-endian
// @return decoded data length or -ve error val
ssize_t lz4_decode_block_fast(void* out_void, const size_t out_len, const void* in_void, const size_t in_len) {
  register u8* restrict out_start = (u8*)out_void;
  register u8* restrict out = (u8*)out_void;
  u8* const out_limit = out + out_len;
  
  register const u8* restrict in = (const u8*)in_void;
  const u8* const in_limit = in + in_len;

  // Output buffer limit for speculative lookahead
  register u8* const out_fast_limit = out_limit - OUT_LOOKAHEAD;

  // Input buffer limit for speculative lookahead
  register const u8* const in_fast_limit = in_limit - IN_LOOKAHEAD;

  // Fast mode with speculative look-ahead
  // Note we could go further and speculatively read some input before doing these
  //   bounds checks, but for now hope that hardware speculative execution is
  //   sufficient.
  while(likely(out < out_fast_limit && in < in_fast_limit)) {
    u8 lits_len_match_len_token = *in++;
    size_t lits_len = token_to_lits_len(lits_len_match_len_token);
    register size_t match_len = token_to_match_len(lits_len_match_len_token);

    // Speculatively read and write 16 bytes of literals assuming lit-len < 15.
    u64 lits1 = *(const u64*)(in+0);
    *(u64*)(out+0) = lits1;
    u64 lits2 = *(const u64*)(in + sizeof(u64));
    *(u64*)(out+sizeof(u64)) = lits2;

    in += lits_len;
    out += lits_len;

    // Speculatively read match offset assuming lit-len < 15.
    // It's a pity that the match offset in lz4 format does not immediately follow the
    //   initial lengths token. If that were the case then this would not be speculative.
    size_t match_offset = *(const u16*)in;
    in += MATCH_OFFSET_LEN;
    
    // We will check that this is in-range below...
    u8* match = out - match_offset;

    // If this is a long literal then most of the above speculation is incorrect and
    // we need to read the long literals length and redo everything.
    // By far the common case is short literals length (<15) ~97%.
    if(unlikely(lits_len == LONG_LITS_LEN)) {
      // Reverse input back to the start of the lit length extension
      in -= 15/*lits_len*/ + MATCH_OFFSET_LEN;
      const u8* orig_in = in - 1;
      // Reverse output back to before the speculative literals
      out -= 15/*lits_len*/;

      // OK not to check input buffer overflow here cos the first lit-len extension
      // byte is definitely within the 16-bytes allowed for literals look-ahead.
      u8 lits_len_extension = *in++;
      lits_len += lits_len_extension;

      while(unlikely(lits_len_extension == LITS_LEN_EXTENSION_EXTRA)) {
	if(in_fast_limit <= in) {
	  // Bail to slow mode but go back to start of current sequence first
	  in = orig_in;
	  goto slow;
	}
	lits_len_extension = *in++;
	lits_len += lits_len_extension;
      }

      // If we're too close to the buffer end then bail to slow mode.
      // Note this is more conservative than necessary for "in".
      if(unlikely(in_fast_limit <= in + lits_len || out_fast_limit <= out + lits_len)) {
	  in = orig_in;
	  goto slow;
      }

      memcpy(out, in, lits_len);

      in += lits_len;
      out += lits_len;

      match_offset = *(const u16*)in;
      in += MATCH_OFFSET_LEN;
    
      // We will check that this is in-range below...
      match = out - match_offset;
    }

    // We are now at the match. At this stage:
    //   in    - points to (optional) match length extension in the current
    //           sequence, or the next sequence start.
    //   out   - after the (optional) literals
    //   match - the source of the match string, not yet bounds-checked

    // Sanity check that the match is within the buffer - this can be avoided once we're
    //   more than 64KiB into the block but is it worth it?
    // TODO - this can underflow out_start :(
    if(unlikely(match < out_start)) {
      return -LZ4_DECODE_ERR_MATCH_OFFSET_TOO_LARGE;
    }

    // Speculatively read and write 16 bytes of match
    u64 matches1 = *(const u64*)(match+0);
    *(u64*)(out+0) = matches1;
    u64 matches2 = *(const u64*)(match + sizeof(u64));
    *(u64*)(out+sizeof(u64)) = matches2;

    // Fast path ~80%
    // Hrmm, don't like the 2nd condition
    if(likely(match_len <= 16 && match + 8 <= out)) {
      out += match_len;
      continue;
    }

    // Handle match length extension.
    // This is the minority case but somewhat common ~25%
    if(unlikely(match_len == LONG_MATCH_LEN)) {
      // Compute the start of the input sequence in case we need to bail to slow mode
      const u8* orig_in =
	in
	- MATCH_OFFSET_LEN
	- ((lits_len + 256 - 15)/256)/*lits len extension*/
	- lits_len
	- LITS_LEN_MATCH_LEN_TOKEN_SIZE;
      
      // Safe cos include in input lookahead
      size_t match_len_extension = *in++;
      match_len += match_len_extension;
      while(unlikely(match_len_extension == MATCH_LEN_EXTENSION_EXTRA)) {
	if(in_fast_limit <= in) {
	  // Bail to slow mode but go back to start of current sequence first
	  in = orig_in;
	  out -= lits_len;
	  goto slow;
	}
	match_len_extension = *in++;
	match_len += match_len_extension;
      }
      // If we're too close to the buffer end then bail to slow mode.
      if(unlikely(out_fast_limit <= out + match_len)) {
	  in = orig_in;
	  out -= lits_len;
	  goto slow;
      }
    }

    // Here we either have a long'ish match - > 16 bytes, or we have an overlap match of any length

    // Is the match an overlap?
    // By far the common case is no overlap ~97%.
    register u8* match_limit = match + match_len;
    if(likely(match_limit <= out || match + sizeof(u64) <= out)) {
      // No problematic overlap, long match > 16 bytes
      match += 16;
      out += 16;
      while(likely(match < match_limit)) {
	u64 matches1 = *(const u64*)(match+0);
	*(u64*)(out+0) = matches1;
	u64 matches2 = *(const u64*)(match + sizeof(u64));
	*(u64*)(out+sizeof(u64)) = matches2;

	match += 16;
	out += 16;
      }
      
      // Correct speculative over-run
      out -= (match - match_limit);
      
    } else {
      // Overlap < 8 bytes - can't copy u64 (8 bytes) at a time.
      // Dominated by offset == 1 (byte fill) then to a lesser extent by
      //   offset == 4 (4-byte fill) and offset == 2 (2-byte fill).
      static const u8 is_aligned_fill[] = { 0, 1, 1, 0, 1, 0, 0, 0 };
      size_t offset = out - match;
      if(likely(is_aligned_fill[offset])) {
	u64 match_pattern;
	if(likely(offset == 1)) {
	  u8 u8_pattern = *match;
	  // TODO - is multiply faster than shifting etc? Could also do table look-up;
	  match_pattern = ((u64)u8_pattern) * 0x0101010101010101UL;
	} else if(likely(offset == 4)) {
	  u32 u32_pattern = *(u32*)match;
	  match_pattern = (u64)u32_pattern | ((u64)u32_pattern << 32);
	} else {
	  // offset == 2
	  u16 u16_pattern = *(u16*)match;
	  // TODO - is multiply faster than shifting etc?
	  match_pattern = ((u64)u16_pattern) * 0x0001000100010001UL;
	}

	// Fill with the match pattern.
	u8* out_match_limit = out + match_len;
	do {
	  *(u64*)(out+0) = match_pattern;
	  *(u64*)(out+sizeof(u64)) = match_pattern;
	  out += 16;
	} while(likely(out < out_match_limit));

	// Fix speculative overrun
	out = out_match_limit;
	
      } else {
	// Poorly aligned overlap - let memmove() deal with it.
	memmove(out, match, match_len);
	out += match_len;
      }
    }
  }

  // Slow mode with no speculative look-ahead for end of buffers where speculative
  //   look-ahead would overrun input or output buffers.
 slow: {
    size_t out_so_far = out - out_start;
    size_t in_so_far = in - (const u8*)in_void;
    
    ssize_t slow_rc = lz4_decode_block_default(out, out_len - out_so_far, in, in_len - in_so_far);
    
    if(slow_rc < 0) {
      // Error code
      return slow_rc;
    }
    
    return out_so_far + slow_rc;
  }
}
