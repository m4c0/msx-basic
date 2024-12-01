#pragma leco tool

import jojo;
import jute;
import hashley;
import parser;
import silog;

static ast::nodes g_program {};
static int g_cur_line {};

static void print(const ast::node & n) {
  switch (n.type) {
    case ast::type::string:
      silog::log(silog::info, "%.*s",
         static_cast<unsigned>(n.content.size()),
         n.content.data());
      break;
    default: silog::die("cannot print node type: %d", n.type);
  }
}

static void run() {
  const auto & line = (*g_program.seek(g_cur_line).children)[0];
  switch (line.type) {
    case ast::type::print: print((*line.children)[0]); break;
    default: silog::die("invalid node type: %d", line.type);
  }
  g_cur_line++;
}

static void compile(jute::view fname) {
  auto src = jojo::read_cstr(fname);
  g_program = parse(fname, src);

  while (g_cur_line < g_program.size()) run();
}

int main(int argc, char ** argv) try {
  compile("example1.bas");
  compile("example2.bas");
} catch (...) {
  return 1;
}
