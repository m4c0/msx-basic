#pragma leco tool

import jojo;
import jute;
import hai;
import hashley;
import parser;
import silog;

enum class var_type {
  number,
  string,
};
struct var {
  var_type type;
  int integer;
  jute::heap str;
};

static ast::nodes g_program {};
static int g_cur_line {};
static hai::chain<var> g_vars { 128 };
static hashley::niamh g_var_idx { 113 };

static var eval(const ast::node & n);

static var eval_binop(const ast::node & n) {
  auto lhs = eval((*n.children)[0]);
  auto rhs = eval((*n.children)[1]);
  if (n.content == "*") {}
  silog::die("cannot eval binary operation '%.*s'",
      static_cast<unsigned>(n.content.size()),
      n.content.data());
}
static var eval_int_cast(const ast::node & n) {
  auto res = eval((*n.children)[0]);
  switch (res.type) {
    default: silog::die("cannot cast to int node type: %d", n.type);
  }
}
static var eval_rnd(const ast::node & n) {
  auto max = eval((*n.children)[0]);
  switch (n.type) {
    default: silog::die("invalid random max type: %d", n.type);
  }
}
static var eval(const ast::node & n) {
  switch (n.type) {
    case ast::type::binop:    return eval_binop(n);
    case ast::type::int_cast: return eval_int_cast(n);
    case ast::type::number:   return { .type = var_type::number, .integer = n.number };
    case ast::type::rnd:      return eval_rnd(n);
    default: silog::die("cannot eval node type: %d", n.type);
  }
}

static void assign(jute::view var, const ast::node & n) {
  auto & idx = g_var_idx[var];
  if (idx == 0) {
    g_vars.push_back({});
    idx = g_vars.size();
  }
  g_vars.seek(idx - 1) = eval(n);
}

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
    case ast::type::assign: assign(line.content, (*line.children)[0]); break;
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
