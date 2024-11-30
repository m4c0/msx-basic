#pragma leco tool

import jojo;
import jute;
import parser;

static void compile(jute::view fname) {
  auto src = jojo::read_cstr(fname);
  auto lines = parse(fname, src);
}

int main(int argc, char ** argv) try {
  compile("example1.bas");
  compile("example2.bas");
} catch (...) {
  return 1;
}
