#pragma leco tool

import jojo;
import jute;
import hashley;
import parser;
import silog;

static void compile(jute::view fname) {
  auto src = jojo::read_cstr(fname);
  auto lines = parse(fname, src);

  unsigned min = ~0;
  hashley::siobhan l_idx { 113 };
  for (auto i = 0; i < lines.size(); i++) {
    auto l_num = lines.seek(i).number;
    l_idx[l_num] = i;
    if (l_num < min) min = l_num;
  }
  silog::trace("start at", min);
  silog::trace("line no", l_idx[min]);
  silog::trace("parsed lines", lines.size());
}

int main(int argc, char ** argv) try {
  compile("example1.bas");
  compile("example2.bas");
} catch (...) {
  return 1;
}
