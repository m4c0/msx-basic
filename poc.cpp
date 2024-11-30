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

  class expr_node : public virtual node {
    node_ptr m_expr {};
  public:
    explicit expr_node() = default;
    explicit expr_node(node * e) : m_expr { e } {}
  };
  class bin_expr_node : public node {
    node_ptr m_a {};
    node_ptr m_b {};
  public:
    explicit constexpr bin_expr_node(node * a, node * b) : m_a { a }, m_b { b } {}
  };
  class num_node : public virtual node {
    int m_n;
  public:
    explicit num_node(int n) : m_n { n } {}
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
  struct pset : bin_expr_node {
    using bin_expr_node::bin_expr_node;
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

  struct go_to : num_node {
    using num_node::num_node;
  };
  struct screen : num_node {
    using num_node::num_node;
  };

  struct line : virtual num_node, virtual expr_node {
    line(int num, node * expr) : num_node{num}, expr_node{expr} {}
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

static ast::node * do_pset() {
  g_ts.match(token::paren::L, "expecting '('");
  auto a = do_expr();
  g_ts.match(token::sym::COMMA, "expecting ','");
  auto b = do_expr();
  g_ts.match(token::paren::R, "expecting ')'");
  return new ast::pset { a, b };;
}

static ast::node * do_goto() {
  auto t = g_ts.take();
  if (t.type != token::number) fail("unsupported token for goto", t);
  return new ast::go_to { atoi(t) };
}

static ast::node * do_screen() {
  auto t = g_ts.take();
  if (t.type != token::number) fail("unsupported screen mode", t);
  return new ast::screen { atoi(t) };
}

static ast::node * do_call() {
  g_ts.match(token::paren::L, "expecting '('");
  auto n = do_expr();
  g_ts.match(token::paren::R, "expecting ')'");
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
  g_ts.match(token::sym::EQ, "expecting '=' after identifier");
  return new ast::assign { var, do_expr() };
}

static ast::node * do_stmt() {
  auto t = g_ts.take();
  if (t.type == token::identifier) return do_assign(t.content);
  else if (t == token::kw::GOTO)   return do_goto();
  else if (t == token::kw::PRINT)  return do_print();
  else if (t == token::kw::PSET)   return do_pset();
  else if (t == token::kw::SCREEN) return do_screen();
  else fail("unexpected token type", t);
}

static ast::node * do_line() {
  auto l_num = g_ts.take();
  if (l_num.type == token::eof) return nullptr;
  if (l_num.type != token::number) fail("line starting without a number", l_num);

  auto expr = do_stmt();

  auto eol = g_ts.take();
  if (eol.type == token::newline) return new ast::line(atoi(l_num), expr);
  if (eol.type == token::eof) return new ast::line(atoi(l_num), expr);
  fail("expecting end of line after statement", eol);
}

static ast::node_ptr parse_line() {
  return ast::node_ptr { do_line() };
}

static void compile(jute::view fname) {
  auto src = jojo::read_cstr(fname);
  g_ts = tokenise(fname, src);
  while (parse_line()) {}
}

int main(int argc, char ** argv) try {
  compile("example1.bas");
  compile("example2.bas");
} catch (...) {
  return 1;
}
