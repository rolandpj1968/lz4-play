for f in `ls ~/tmp/compression/mem.raw.by-2mb/page-100?.raw.lz4-9`; do echo $f; ls -als $f; echo; ./lz4-parse $f; echo; done > parse.out

cat parse.out | awk '/ lits / { n_lit_lines += 1; n_lits += $3 } END { print NR, "lines", n_lit_lines, "literal lines - avg. lits per line", n_lits/n_lit_lines; }'

cat parse.out | awk '/ matches / { n_match_lines += 1; n_matches += $5 } END { print NR, "lines", n_match_lines, "match lines - avg. match len per line", n_matches/n_match_lines; }'

cat parse.out | awk '/ matches / { n_match_lines += 1; match_len = $5; offset = $7; if(offset == 1) { n_one_offsets += 1; } if(offset < 16) { n_small_offsets += 1; } if(offset < match_len) { n_overlaps += 1; } } END { print NR, "lines", n_match_lines, "match lines - n_one_offsets", n_one_offsets, "n_small_offsets", n_small_offsets, "n_overlaps", n_overlaps, n_overlaps/n_match_lines; }'

cat parse.out | awk '/ lits / { n_lit_lines += 1; lits_len = $3; n_lits += $3; n_lits_lens[lits_len] += 1; } END { print NR, "lines", n_lit_lines, "literal lines - avg. lits per line", n_lits/n_lit_lines; accum_n_lits_len = 0; for(i = 0; i < 16; i += 1) { accum_n_lits_len += n_lits_lens[i]; print "lits len", i, "count", n_lits_lens[i], n_lits_lens[i]/n_lit_lines, "accum", accum_n_lits_len, accum_n_lits_len/n_lit_lines} }'

cat parse.out | awk '/ lits / { n_lit_lines += 1; lits_len = $3; n_lits += $3; n_lits_lens[lits_len] += 1; } END { print NR, "lines", n_lit_lines, "literal lines - avg. lits per line", n_lits/n_lit_lines; accum_n_lits_len = 0; for(i = 0; i <= 32; i += 1) { accum_n_lits_len += n_lits_lens[i]; accum_lits_len += i * n_lits_lens[i]; print "lits len", i, "count", n_lits_lens[i], n_lits_lens[i]/n_lit_lines, "accum", accum_n_lits_len, accum_n_lits_len/n_lit_lines, "avg-lits-per-seq", accum_lits_len/accum_n_lits_len} }'

cat parse.out | awk '/ matches / { n_match_lines += 1; match_len = $5; n_matches += $5; n_match_lens[match_len] += 1; } END { print NR, "lines", n_match_lines, "match lines - avg. match len per line", n_matches/n_match_lines; for(i = 0; i < 20; i += 1) { accum_n_match_len += n_match_lens[i]; print "match len", i, "count", n_match_lens[i], n_match_lens[i]/n_match_lines, "accum", accum_n_match_len, accum_n_match_len/n_match_lines} }'

cat parse.out | awk '/ matches / { n_match_lines += 1; offset = $7; n_matches += $5; n_offsets[offset] += 1; } END { print NR, "lines", n_match_lines, "match lines - avg. match len per line", n_matches/n_match_lines; for(i = 0; i < 20; i += 1) { accum_n_offsets += n_offsets[i]; print "offset", i, "offset counts", n_offsets[i], n_offsets[i]/n_match_lines, "accum", accum_n_offsets, accum_n_offsets/n_match_lines} }'

cat parse.out | awk '/ matches / { n_match_lines += 1; match_len = $5; offset = $7; n_matches += $5; if(match_len > 8) { n_matches_gt_8 += 1; n_offsets[offset] += 1; } } END { print NR, "lines", n_match_lines, "match lines", n_matches_gt_8, "matches > 8 bytes"; for(i = 0; i < 8; i += 1) { accum_n_offsets += n_offsets[i]; print "offset", i, "offset counts", n_offsets[i], n_offsets[i]/n_matches_gt_8, "accum", accum_n_offsets, accum_n_offsets/n_matches_gt_8} }'


