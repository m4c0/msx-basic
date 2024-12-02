export module parser;

import hai;
import jute;
import lexer;

export namespace ast {
  enum class type {
    nil,
    assign,
    binop,
    go_to,
    int_cast,
    integer,
    line,
    print,
    pset,
    rnd,
    screen,
    string,
    variable,
  };
  struct node {
    type type {};
    int number {};
    jute::view content {};
    hai::sptr<hai::array<node>> children {};
  };
  using nodes = hai::chain<ast::node>;
}

static constexpr int atoi(jute::view str) {
  auto p = str[0] == '-' ? str.subview(1).after : str;;
  int res = 0;
  for (auto i = 0; i < p.size(); i++) {
    res = res * 10 + (p[i] - '0');
  }
  return str[0] == '-' ? -res : res;
}
static_assert(atoi("10") == 10);
static_assert(atoi("-10") == -10);

namespace ast {
  static constexpr auto c(auto ... n) {
    using arr_t = hai::array<ast::node>;
    auto arr = arr_t::make(n...);
    return hai::sptr { new arr_t{traits::move(arr)} };
  }
  static constexpr auto assign(jute::view v, node r) {
    return node { .type = type::assign, .content = v, .children = c(r) };
  }
  static constexpr auto line(jute::view n, node r) {
    return node { .type = type::line, .number = atoi(n), .children = c(r) };
  }

  static constexpr auto binop(node lhs, jute::view op, node rhs) {
    return node { .type = type::binop, .content = op, .children = c(lhs, rhs) };
  }

  static constexpr auto number(type t, jute::view v) {
    return node { .type = t, .number = atoi(v), .content = v };
  }
  static constexpr auto view(type t, jute::view v) {
    return node { .type = t, .content = v };
  }

  static constexpr auto unary(type t, node r) {
    return node { .type = t, .children = c(r) };
  }
  // static constexpr auto binary(type t, node a, node b) {
  //   return node { .type = t, .children = c(a, b) };
  // }
  static constexpr auto ternary(type t, node a, node b, node c) {
    return node { .type = t, .children = ast::c(a, b, c) };
  }

  static constexpr auto print(node n) { return unary(type::print, n); }
  static constexpr auto string(jute::view n) { return view(type::string, n); }
}

static token::stream g_ts {};

static ast::node do_expr();

static ast::node do_print() {
  auto t = g_ts.peek();
  if (t.type == token::newline) return ast::print(ast::string(""));
  if (t.type == token::string) return ast::print(ast::string(g_ts.take().content));
  return ast::print(do_expr());
}

static ast::node do_pset() {
  g_ts.match(token::paren::L, "expecting '('");
  auto a = do_expr();
  g_ts.match(token::sym::COMMA, "expecting ','");
  auto b = do_expr();
  g_ts.match(token::paren::R, "expecting ')'");

  ast::node c = number(ast::type::integer, "15");
  if (g_ts.peek() == token::sym::COMMA) {
    g_ts.take();
    c = do_expr();
  }
  return ternary(ast::type::pset, a, b, c);
}

static ast::node do_goto() {
  auto t = g_ts.take();
  if (t.type != token::integer) fail("unsupported token for goto", t);
  return number(ast::type::go_to, t.content);
}

static ast::node do_screen() {
  auto t = g_ts.take();
  if (t.type != token::integer) fail("unsupported screen mode", t);
  return number(ast::type::screen, t.content);
}

static ast::node do_call() {
  g_ts.match(token::paren::L, "expecting '('");
  auto n = do_expr();
  g_ts.match(token::paren::R, "expecting ')'");
  return n;
}

static ast::node do_lhs() {
  auto lhs = g_ts.take();
  if (lhs == token::kw::INT) return unary(ast::type::int_cast, do_call());
  if (lhs == token::kw::RND) return unary(ast::type::rnd, do_call());
  if (lhs.type == token::integer) return number(ast::type::integer, lhs.content);
  if (lhs.type != token::identifier) fail("invalid token in LHS of expression", lhs);
  return view(ast::type::variable, lhs.content);
}

static ast::node do_expr() {
  auto lhs = do_lhs();

  if (g_ts.peek().type != token::oper) return lhs;
  auto op = g_ts.take();

  auto rhs = do_expr();
  return binop(lhs, op.content, rhs);
}

static ast::node do_assign(jute::view var) {
  g_ts.match(token::sym::EQ, "expecting '=' after identifier");
  return assign(var, do_expr());
}

static ast::node do_stmt() {
  auto t = g_ts.take();
  if (t.type == token::identifier) return do_assign(t.content);
  else if (t == token::kw::GOTO)   return do_goto();
  else if (t == token::kw::PRINT)  return do_print();
  else if (t == token::kw::PSET)   return do_pset();
  else if (t == token::kw::SCREEN) return do_screen();
  else fail("unexpected token type", t);
}

static ast::node do_line() {
  auto l_num = g_ts.take();
  if (l_num.type == token::eof) return {};
  if (l_num.type != token::integer) fail("line starting without a number", l_num);

  auto expr = do_stmt();

  auto eol = g_ts.take();
  if (eol.type == token::newline) return ast::line(l_num.content, expr);
  if (eol.type == token::eof)     return ast::line(l_num.content, expr);
  fail("expecting end of line after statement", eol);
}

export auto parse(jute::view fname, const hai::cstr & src) {
  g_ts = tokenise(fname, src);

  ast::nodes lines { 1000 };
  while (true) {
    auto n = do_line();
    if (n.type == ast::type::nil) break;
    lines.push_back(n);
  }

  // Bubble-sort by line number
  for (auto a = lines.begin(); a != lines.end(); ++a) {
    auto b = a;
    for (++b; b != lines.end(); ++b) {
      if ((*a).number > (*b).number) {
        auto tmp = *a;
        *a = *b;
        *b = tmp;
      }
    }
  }

  return lines;
}
