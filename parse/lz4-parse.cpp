#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <streambuf>
#include <string>
#include <sstream>
#include <utility>

#include "decode.h"
#include "util.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<double> dsec;

const size_t MiB = 1 << 20;
const size_t KiB = 1 << 10;

const double ms_per_s = 1000.0;
  
namespace Lz4 {

  // https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md
  namespace Frame {

    namespace Flg {
      const u8 VERSION_SHIFT = 6;
      const u8 BLOCK_INDEP_SHIFT = 5;
      const u8 BLOCK_CHECKSUM_SHIFT = 4;
      const u8 CONTENT_SIZE_SHIFT = 3;
      const u8 CONTENT_CHECKSUM_SHIFT = 2;
      const u8 RESERVED_1_SHIFT = 1;
      const u8 DICT_ID_SHIFT = 0;

      const u8 version(const u8 flg) { return flg >> VERSION_SHIFT; }
      const u8 VERSION_01 = 0x1;

      const u8 BLOCK_INDEP_FLAG = 1 << BLOCK_INDEP_SHIFT;
      const u8 BLOCK_CHECKSUM_FLAG = 1 << BLOCK_CHECKSUM_SHIFT;
      const u8 CONTENT_SIZE_FLAG = 1 << CONTENT_SIZE_SHIFT;
      const u8 CONTENT_CHECKSUM_FLAG = 1 << CONTENT_CHECKSUM_SHIFT;
      const u8 RESERVED_1_FLAG = 1 << RESERVED_1_SHIFT;
      const u8 DICT_ID_FLAG = 1 << DICT_ID_SHIFT;

      bool flag_is_set(const u8 flags, const u8 flag) {
	return (flags & flag) != 0;
      }
      
    } // namespace Flg

    namespace Bd {
      const u8 RESERVED_7_SHIFT = 7;

      bool reserved_7(const u8 bd) { return (bd >> RESERVED_7_SHIFT) != 0; }
      
      const u8 BLOCK_MAX_SIZE_SHIFT = 4;
      const u8 BLOCK_MAX_SIZE_WIDTH = 3;
      const u8 BLOCK_MAX_SIZE_MASK = (1 << BLOCK_MAX_SIZE_WIDTH) - 1;

      u8 block_max_size(const u8 bd) { return (bd >> BLOCK_MAX_SIZE_SHIFT) & BLOCK_MAX_SIZE_MASK; }

      const u8 RESERVED_3_2_1_0_SHIFT = 0;
      const u8 RESERVED_3_2_1_0_WIDTH = 4;
      const u8 RESERVED_3_2_1_0_MASK = (1 << RESERVED_3_2_1_0_WIDTH) - 1;

      u8 reserved_3_2_1_0(const u8 bd) { return (bd >> RESERVED_3_2_1_0_SHIFT) & RESERVED_3_2_1_0_MASK; }
      
    } // namespace Bd

    struct Descriptor {
      const u8 flg;
      const u8 bd;

      const u64 content_size;

      const u32 dict_id;

      const u8 hc;

      Descriptor(const u8 flg, const u8 bd, const u64 content_size, const u32 dict_id, const u8 hc)
	: flg(flg), bd(bd), content_size(content_size), dict_id(dict_id), hc(hc) {}

      u8 flg_version() const {
	return Flg::version(flg);
      }
      
      bool flg_is_set(const u8 flag) const {
	return Flg::flag_is_set(flg, flag);
      }

      bool bd_reserved_7() const {
	return Bd::reserved_7(bd);
      }

      u8 bd_block_max_size() const {
	return Bd::block_max_size(bd);
      }

      u8 bd_reserved_3_2_1_0() const {
	return Bd::reserved_3_2_1_0(bd);
      }
    };

    const u32 LZ4_FRAME_MAGIC = 0x184d2204;
    
    struct Header {
      const size_t len;
      const u32 magic;
      const Descriptor descriptor;

      Header(size_t len, u32 magic, const u8 flg, const u8 bd, const u64 content_size, const u32 dict_id, const u8 hc)
	: len(len), magic(magic), descriptor(Descriptor(flg, bd, content_size, dict_id, hc)) {}
    };

    struct Trailer {
      const u32 content_checksum;

      Trailer(const u32 content_checksum)
	: content_checksum(content_checksum) {}
    };
    
  } // namespace Frame

  // https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md
  namespace Block {

    struct Header {
      const u32 block_size;

      Header(const u32 block_size)
	: block_size(block_size) {}

      bool is_endmark() const {
	return block_size == 0;
      }

      bool is_compressed() const {
	return (block_size & 0x80000000) == 0;
      }

      // Note - does not include the block checksum if present.
      u32 data_length() const {
	return block_size & 0x7fffffff;
      }
    };

    struct Trailer {
      const u32 block_checksum;

      Trailer(const u32 block_checksum)
	: block_checksum(block_checksum) {}
    };
    
  } // namespace Block
  
  namespace Parse {

    Frame::Header parse_header(const u8* buf, size_t buf_len) {
      
      const int min_header_len = sizeof(Frame::Header::magic) + sizeof(Frame::Descriptor::flg)
	+ sizeof(Frame::Descriptor::bd) + sizeof(Frame::Descriptor::hc);
      
      if(buf_len < min_header_len) {
	throw std::string("Input buffer too short for minimum lz4 frame header");
      }

      size_t header_len = min_header_len;
	 
      const u32 magic = u32_at_offset(buf, 0);

      if(magic != Frame::LZ4_FRAME_MAGIC) {
	throw std::string("Invalid lz frame magic number");
      }

      buf += sizeof(Frame::Header::magic);

      const u8 flg = *buf++;

      if(Frame::Flg::version(flg) != Frame::Flg::VERSION_01) {
	throw std::string("Unrecognized lz4 frame version number");
      }

      if(Frame::Flg::flag_is_set(flg, Frame::Flg::RESERVED_1_FLAG)) {
      	throw std::string("Reserved bit 1 in lz4 flg field is not 0");
      }
      
      const u8 bd = *buf++;

      if(Frame::Bd::reserved_7(bd) != 0) {
	throw std::string("Reserved bit 7 in lz4 bd field is not 0");
      }

      if(Frame::Bd::reserved_3_2_1_0(bd) != 0) {
	throw std::string("Reserved bits 3-0 in lz4 bd field are not 0");
      }

      // TODO - validate block_max_size

      u32 content_size = 0;

      if(Frame::Flg::flag_is_set(flg, Frame::Flg::CONTENT_SIZE_FLAG)) {
	header_len += sizeof(Frame::Descriptor::content_size);

	if(buf_len < header_len) {
	  throw std::string("Input buffer too short for lz4 frame header with content size present");
	}

	content_size = u64_at_offset(buf, 0);

	buf += sizeof(Frame::Descriptor::content_size);
      }

      u32 dict_id = 0;
      
      if(Frame::Flg::flag_is_set(flg, Frame::Flg::DICT_ID_FLAG)) {
	header_len += sizeof(Frame::Descriptor::dict_id);

	if(buf_len < header_len) {
	  throw std::string("Input buffer too short for lz4 frame header with dictionary ID present");
	}

	dict_id = u32_at_offset(buf, 0);

	buf += sizeof(Frame::Descriptor::dict_id);
      }
      
      const u8 hc = *buf++;

      // TODO validate header checksum (hc)

      return Frame::Header(header_len, magic, flg, bd, content_size, dict_id, hc);
      
    }

    //template <typename BlockFn> 
    Block::Header parse_block_header(const u8* buf, size_t buf_len) {
      const int min_header_len = sizeof(Block::Header::block_size);
      
      if(buf_len < min_header_len) {
	throw std::string("Input buffer too short for minimum lz4 block header");
      }

      const u32 block_size = u32_at_offset(buf, 0);
      
      return Block::Header(block_size);
    }
  } // namespace Parse
  
} // namespace Lz4

// @return sequence size
size_t show_sequence(const u8* buf, size_t buf_len) {
  const u8* seq_start = buf;
  
  if(buf_len <= 0) {
    throw std::string("Sequence with no available data");
  }
  
  u8 sizes = *buf++;
  buf_len --;
  size_t lits_len = sizes >> 4;
  size_t match_len = (sizes & 0xf) + 4;

  if(lits_len == 15) {
    u8 add_lits_len;
    do {
      if(buf_len <= 0) {
	throw std::string("Sequence ran out of bytes for lit len");
      }
      add_lits_len = *buf++;
      buf_len--;
      
      lits_len += add_lits_len;
    }
    while(add_lits_len == 255);
  }

  // skip lits
  buf += lits_len;
  buf_len -= lits_len;

  // Last sequence does not have a match
  if(buf_len == 0) {
    printf("    sequence: lits %lu no-match\n", lits_len);

    return buf - seq_start;
  }

  if(buf_len <= sizeof(u16)) {
    throw std::string("Sequence ran out of bytes for match offset");
  }
  
  u16 offset = u16_at_offset(buf, 0);

  buf += sizeof(u16);
  buf_len -= sizeof(u16);

  if(match_len == 19) {
    u8 add_match_len;
    do {
      if(buf_len <= 0) {
	throw std::string("Sequence ran out of bytes for match len");
      }
      add_match_len = *buf++;
      buf_len--;
      match_len += add_match_len;
    }
    while(add_match_len == 255);
  }

  printf("    sequence: lits %lu matches %lu match-offset %u\n", lits_len, match_len, offset);

  return buf - seq_start;
}

void show_sequences(const u8* buf, size_t block_len) {
  while(block_len > 0) {
    size_t seq_len = show_sequence(buf, block_len);
    if(block_len < seq_len) {
      throw std::string("Sequence bigger than remaining block size");
    }
    buf += seq_len;
    block_len -= seq_len;
  }
}

int main(int argc, char* argv[]) {
  if(argc != 2) {
    fprintf(stderr, "%s <in-file>\n", argv[0]);
    exit(1);
  }

  auto t0 = Time::now();
  
  char* buf_file = argv[1];
  std::string buf_str = Util::slurp(buf_file);

  const u8* buf = (const u8*)buf_str.c_str();
  size_t buf_len = buf_str.length();

  auto t1 = Time::now();
  dsec ds1 = t1 - t0;
  double secs1 = ds1.count();
  
  printf("Read %s length %zu in %7.3lfms\n", buf_file, buf_len, secs1*1000.0);

  try {
    Lz4::Frame::Header header = Lz4::Parse::parse_header(buf, buf_len);

    printf("lz4 header: len %zu magic 0x%04x descriptor flg 0x%02x bd 0x%02x content-size %lu dict-id %u hc 0x%02x\n",
	   header.len, header.magic, header.descriptor.flg, header.descriptor.bd,
	   header.descriptor.content_size, header.descriptor.dict_id, header.descriptor.hc);

    buf += header.len;
    buf_len -= header.len;
    
    for(int block_no = 0;; block_no++) {
      Lz4::Block::Header block_header = Lz4::Parse::parse_block_header(buf, buf_len);

      printf("  block %d: is-endmark %s is-compressed %s data-length %u\n", block_no, (block_header.is_endmark() ? "true" : "false"), (block_header.is_compressed() ? "true" : "false"), block_header.data_length());
      if(block_header.is_endmark()) {
	buf += sizeof(Lz4::Block::Header::block_size);
	buf_len -= sizeof(Lz4::Block::Header::block_size);
	break;
      }

      buf += sizeof(Lz4::Block::Header::block_size);
      buf_len -= sizeof(Lz4::Block::Header::block_size);

      size_t block_size = block_header.data_length() + (header.descriptor.flg_is_set(Lz4::Frame::Flg::BLOCK_CHECKSUM_FLAG) ? 4 : 0);

      if(buf_len < block_size) {
	throw std::string("Block size is greater than remaining buffer");
      }

      if(block_header.is_compressed()) {
	show_sequences(buf, block_header.data_length());

	// Warm up decode
	static u8 out_buf[4*1024*1024];
	//ssize_t raw_len = lz4_decode_block_default(out_buf, sizeof(out_buf), buf, block_header.data_length());
	ssize_t raw_len = lz4_decode_block_fast(out_buf, sizeof(out_buf), buf, block_header.data_length());
	printf("    block %d: decode-len %ld\n", block_no, raw_len);

	if(raw_len >= 0) {
	  // Time decode
	  auto t0 = Time::now();
	  
	  const unsigned n_iters = 256;
	  for(unsigned i = 0; i < n_iters; i++) {
	    //ssize_t raw_len2 = lz4_decode_block_default(out_buf, sizeof(out_buf), buf, block_header.data_length());
	    ssize_t raw_len2 = lz4_decode_block_fast(out_buf, sizeof(out_buf), buf, block_header.data_length());
	    if(raw_len2 != raw_len) {
	      printf("                      abort bad raw len %ld expecting %ld\n", raw_len2, raw_len);
	      break;
	    }
	  }
	  auto t1 = Time::now();
	  dsec ds1 = t1 - t0;
	  double secs1 = ds1.count();
	  
	  double ms = secs1 * ms_per_s;
	  size_t copy_len = (size_t)raw_len;
	  double mib_per_s = copy_len*n_iters/MiB / secs1;
	  
	  printf("decompressed %zu bytes %u times in %9.3lfms - %10.3lfMiB/s\n", copy_len, n_iters, ms, mib_per_s);
	}
      }

      buf += block_size;
      buf_len -= block_size;
    }

    printf("buf len left %lu\n", buf_len);
  }
  catch(const std::string msg) {
    printf("Error parsing lz4 frame: %s\n", msg.c_str());
  }
}  
