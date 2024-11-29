#pragma leco tool

import hai;
import jojo;
import jute;
import lexer;
import silog;

namespace ast {
  class node {
  };
  using node_ptr = hai::uptr<node>;

  class expr_node : public node {
    node_ptr m_expr {};
  public:
    explicit constexpr expr_node() = default;
    explicit constexpr expr_node(node * e) : m_expr { e } {}
  };
  class view_node : public node {
    jute::view m_n {};
  public:
    explicit constexpr view_node(jute::view n) : m_n { n } {}
  };

  class assign : public node {
    jute::view m_var {};
    node_ptr m_val {};
  public:
    explicit constexpr assign(jute::view v, node * r) : m_var { v }, m_val { r } {}
  };

  class binop : public node {
    node_ptr m_lhs {};
    char m_op {};
    node_ptr m_rhs {};
  public:
    explicit constexpr binop(node * l, char op, node * r) : m_lhs { l }, m_op { op }, m_rhs { r } {}
  };

  struct int_cast : expr_node {
    using expr_node::expr_node;
  };
  struct print : expr_node {
    using expr_node::expr_node;
  };
  struct rnd : expr_node {
    using expr_node::expr_node;
  };

  struct number : view_node {
    using view_node::view_node;
  };
  struct string : view_node {
    using view_node::view_node;
  };
  struct variable : view_node {
    using view_node::view_node;
  };

  class screen : public node {
    int m_mode;
  public:
    explicit constexpr screen(int n) : m_mode { n } {}
  };
}

static constexpr int atoi(const token::t & t) {
  auto str = t.content;
  auto p = str[0] == '-' ? str.subview(1).after : str;;
  int i = 0;
  for (auto i = 0; i < p.size(); i++) {
    i = i * 10 + (p[i] - '0');
  }
  return str[0] == '-' ? -i : i;
}

static token::stream g_ts {};

static ast::node * do_expr();

static ast::node * do_print() {
  auto t = g_ts.peek();
  if (t.type == token::newline) return new ast::print {};
  if (t.type == token::string) return new ast::string { g_ts.take().content };
  return new ast::print { do_expr() };
}

static ast::node * do_screen() {
  auto t = g_ts.take();
  if (t.type != token::number) fail("unsupported screen mode", t);
  return new ast::screen { atoi(t) };
}

static ast::node * do_call() {
  g_ts.match(token::paren::L, "expecting '(', got");
  auto n = do_expr();
  g_ts.match(token::paren::R, "expecting ')', got");
  return n;
}

static ast::node * do_lhs() {
  auto lhs = g_ts.take();
  if (lhs == token::kw::INT) return new ast::int_cast { do_call() };
  if (lhs == token::kw::RND) return new ast::rnd { do_call() };
  if (lhs.type == token::number) return new ast::number { lhs.content };
  if (lhs.type != token::identifier) fail("invalid token in LHS of expression", lhs);
  return new ast::variable { lhs.content };
}

static ast::node * do_expr() {
  auto lhs = do_lhs();

  if (g_ts.peek().type != token::oper) return lhs;
  auto op = g_ts.take();

  auto rhs = do_expr();
  return new ast::binop { lhs, op.content[0], rhs };
}

static ast::node * do_assign(jute::view var) {
  g_ts.match(token::sym::EQ, "expecting '=' after identifier, got");
  return new ast::assign { var, do_expr() };
}

static ast::node * do_stmt() {
  auto t = g_ts.take();
  if (t.type == token::identifier) return do_assign(t.content);
  else if (t == token::kw::PRINT)  return do_print();
  else if (t == token::kw::SCREEN) return do_screen();
  else fail("unexpected token type", t);
}

static bool parse_line() {
  auto l_num = g_ts.take();
  if (l_num.type == token::eof) return false;
  if (l_num.type != token::number) fail("line starting without a number", l_num);

  do_stmt();

  silog::trace(l_num.content);

  auto eol = g_ts.take();
  if (eol.type == token::newline) return true;
  if (eol.type == token::eof) return false;
  fail("expecting end of like after statement, found", eol);
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
