for f in `ls ~/tmp/compression/mem.raw.by-2mb/page-100?.raw.lz4-9`; do echo $f; ls -als $f; echo; ./lz4-parse $f; echo; done > parse.out

cat parse.out | awk '/ lits / { n_lit_lines += 1; n_lits += $3 } END { print NR, "lines", n_lit_lines, "literal lines - avg. lits per line", n_lits/n_lit_lines; }'

cat parse.out | awk '/ matches / { n_match_lines += 1; n_matches += $5 } END { print NR, "lines", n_match_lines, "match lines - avg. match len per line", n_matches/n_match_lines; }'

cat parse.out | awk '/ matches / { n_match_lines += 1; match_len = $5; offset = $7; if(offset == 1) { n_one_offsets += 1; } if(offset < 16) { n_small_offsets += 1; } if(offset < match_len) { n_overlaps += 1; } } END { print NR, "lines", n_match_lines, "match lines - n_one_offsets", n_one_offsets, "n_small_offsets", n_small_offsets, "n_overlaps", n_overlaps; }'
