#pragma leco tool

import hai;
import jojo;
import jute;
import lexer;
import silog;

static token::stream g_ts {};

static void do_print() {
  auto t = g_ts.peek();
  if (t.type == token::string) {
    g_ts.take();
  } else if (t.type == token::number) {
    g_ts.take();
  } else if (t.type == token::identifier) {
    g_ts.take();
  } else if (t.type == token::newline) {
  } else fail("can't print token", t);
}

static void do_screen() {
  auto t = g_ts.take();
  if (t.type == token::number) {
  } else fail("invalid screen mode", t);
}

static void do_expr();

static void do_call() {
  g_ts.match(token::paren::L, "expecting '(', got");
  do_expr();
  g_ts.match(token::paren::R, "expecting ')', got");
}

static void do_expr() {
  auto lhs = g_ts.take();
  if (lhs == token::kw::INT) return do_call();
  if (lhs == token::kw::RND) return do_call();
  if (lhs.type == token::number) {
  } else if (lhs.type == token::identifier) {
  } else fail("invalid token in LHS of expression", lhs);

  auto op = g_ts.peek();
  if (op.type != token::oper) return;

  auto rhs = g_ts.take();
  if (rhs.type == token::number) {
  } else if (rhs.type == token::identifier) {
  } else fail("invalid token in RHS of expression", rhs);
}

static void do_assign() {
  auto t = g_ts.take();
  if (t.type == token::oper && t.content == "=") do_expr();
  else silog::die("not sure what to do with %s", t.content.cstr().begin());
}

static bool parse_line() {
  auto l_num = g_ts.take();
  if (l_num.type == token::eof) return false;
  if (l_num.type != token::number) silog::die("line starting without a number");
  while (g_ts.peek().type != token::newline) {
    auto t = g_ts.take();
    if (t.type == token::identifier) do_assign();
    else if (t == token::kw::PRINT) do_print();
    else if (t == token::kw::SCREEN) do_screen();
    else fail("unexpected token type at line", l_num);
  }
  silog::trace(l_num.content);
  return g_ts.take().type != token::eof;
}

static void compile(void *, hai::cstr & src) {
  g_ts = tokenise(src);
  while (parse_line()) {}
}

int main(int argc, char ** argv) try {
  jojo::read("example1.bas", nullptr, compile);
  jojo::read("example2.bas", nullptr, compile);
} catch (...) {
  return 1;
}
