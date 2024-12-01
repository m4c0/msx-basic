#pragma leco tool

import jojo;
import jute;
import hai;
import hashley;
import parser;
import rng;
import silog;

enum class var_type {
  nil,
  integer,
  real,
  string,
};
struct var {
  var_type type;
  int integer;
  float real;
  jute::heap str;
};

static ast::nodes g_program {};
static int g_cur_line {};
static hai::chain<var> g_vars { 128 };
static hashley::niamh g_var_idx { 113 };

static constexpr void assert_int(const var & v, const char * msg) {
  if (v.type == var_type::integer) return;
  silog::die("'%s' does not support var type %d", msg, v.type);
}
static constexpr void assert_real(const var & v, const char * msg) {
  if (v.type == var_type::real || v.type == var_type::integer) return;
  silog::die("'%s' does not support var type %d", msg, v.type);
}

static var var_int(int n) {
  return var { .type = var_type::integer, .integer = n, .real = static_cast<float>(n) };
}
static var var_real(float n) {
  return var { .type = var_type::real, .real = n };
}

static var eval(const ast::node & n);

static var eval_binop(const ast::node & n) {
  auto lhs = eval((*n.children)[0]);
  auto rhs = eval((*n.children)[1]);
  if (n.content == "*") {
    assert_real(lhs, "*");
    assert_real(rhs, "*");
    return var_real(lhs.real * rhs.real);
  } 
  silog::die("cannot eval binary operation '%.*s'",
      static_cast<unsigned>(n.content.size()),
      n.content.data());
}
static var eval_int_cast(const ast::node & n) {
  auto res = eval((*n.children)[0]);
  switch (res.type) {
    case var_type::integer: return res;
    case var_type::real:    return var_int(static_cast<int>(res.real));
    default: silog::die("cannot cast to int node type: %d", res.type);
  }
}
static var eval_integer(const ast::node & n) { return var_int(n.number); }
static var eval_rnd(const ast::node & n) {
  auto max = eval((*n.children)[0]);
  switch (max.type) {
    case var_type::integer: return var_real(rng::randf() * max.integer);
    default: silog::die("invalid random max type: %d", max.type);
  }
}
static var eval_var(const ast::node & n) {
  auto idx = g_var_idx[n.content];
  if (idx == 0) silog::die("undefined variable [%.*s]",
      static_cast<unsigned>(n.content.size()),
      n.content.data());
  return g_vars.seek(idx - 1);
}
static var eval(const ast::node & n) {
  switch (n.type) {
    case ast::type::binop:    return eval_binop(n);
    case ast::type::int_cast: return eval_int_cast(n);
    case ast::type::integer:  return eval_integer(n);
    case ast::type::rnd:      return eval_rnd(n);
    case ast::type::variable: return eval_var(n);
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

static void go_to(int n) {
  for (auto i = 0; i < g_program.size(); i++) {
    if (g_program.seek(i).number == n) {
      g_cur_line = i - 1;
      return;
    }
  }
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

static void pset(const ast::node & n) {
  auto x = eval((*n.children)[0]);
  assert_int(x, "pset");
  auto y = eval((*n.children)[1]);
  assert_int(y, "pset");

  silog::log(silog::debug, "TBD: PSET(%d, %d)", x.integer, y.integer);
}

static void screen(int n) {
  silog::log(silog::debug, "TBD: SCREEN %d", n);
}

static void run() {
  const auto & line = (*g_program.seek(g_cur_line).children)[0];
  switch (line.type) {
    case ast::type::assign: assign(line.content, (*line.children)[0]); break;
    case ast::type::go_to:  go_to(line.number); break;
    case ast::type::print:  print((*line.children)[0]); break;
    case ast::type::pset:   pset(line); break;
    case ast::type::screen: screen(line.number); break;
    default: silog::die("invalid node type: %d", line.type);
  }
  g_cur_line++;
}

static void compile(jute::view fname) {
  auto src = jojo::read_cstr(fname);
  g_program = parse(fname, src);
  g_cur_line = 0;

  while (g_cur_line < g_program.size()) run();
}

int main(int argc, char ** argv) try {
  compile("example1.bas");
  compile("example2.bas");
} catch (...) {
  return 1;
}
