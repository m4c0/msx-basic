#pragma leco tool

import hai;
import jojo;
import jute;
import silog;

namespace token {
  enum type {
    nil,
    number,
    string,
    keyword,
    identifier,
    oper,
    parenthesis,
    newline,
    eof,
  };

  struct t {
    type type;
    jute::view content;
  };
  static constexpr bool operator==(const token::t & a, const token::t & b) {
    return a.type == b.type && a.content == b.content;
  }
  [[noreturn]] static void fail(const char * msg, token::t t) {
    silog::die("%s: %s", msg, t.content.cstr().begin());
  }

  class stream {
    hai::chain<t> m_list { 1024 };
    hai::chain<t>::iterator m_it;
  public:
    constexpr void push_back(t t) { m_list.push_back(t); }

    constexpr void reset() { m_it = m_list.begin(); }
    constexpr auto peek() { return *m_it; }
    constexpr auto take() {
      if (m_it == m_list.end()) return t { eof, "" };
      auto & res = *m_it;
      ++m_it;
      return res;
    }

    constexpr void match(const t & exp, const char * err) {
      auto tt = take();
      if (tt != exp) fail(err, tt);
    }
  };

  static t make(type tp, const char * start, const char * end) {
    unsigned sz = end - start;
    return { .type = tp, .content = { start, sz } };
  }
}
namespace token::kw {
  static constexpr t INT    { .type = keyword, .content = "INT" };
  static constexpr t PRINT  { .type = keyword, .content = "PRINT" };
  static constexpr t RND    { .type = keyword, .content = "RND" };
  static constexpr t SCREEN { .type = keyword, .content = "SCREEN" };
}
namespace token::paren {
  static constexpr t L { .type = parenthesis, .content = "(" };
  static constexpr t R { .type = parenthesis, .content = ")" };
}

static bool match(const char *& ptr, jute::view token) {
  for (auto i = 0; i < token.size(); i++) {
    if ((ptr[i] & ~0x20) != token[i]) return false;
  }
  auto e = ptr[token.size()] & ~0x20;
  if (e >= 'A' && e <= 'Z') return false;

  ptr += token.size();
  return true;
}

static constexpr bool is_digit(char c) {
  return c >= '0' && c <= '9';
}
static constexpr bool is_alpha(char c) {
  c &= ~0x20;
  return c >= 'A' && c <= 'Z';
}

static token::stream g_ts {};

static void tokenise(hai::cstr & src) {
  token::stream res {};
  const char * ptr = src.begin();
  while (auto c = *ptr) {
    auto cs = ptr;
    switch (c) {
      case ' ': ptr++; continue;
      case '(': res.push_back(token::paren::R); ptr++; continue;
      case ')': res.push_back(token::paren::L); ptr++; continue;
      case '\n':
        res.push_back(token::make(token::newline, cs, ++ptr));
        continue;
      case '=':
      case ',':
      case '+': case '-': case '*': case '/':
        res.push_back(token::make(token::oper, cs, ++ptr));
        continue;
      case '"':
        do { ptr++; } while (*ptr && *ptr != '"');
        if (!*ptr) silog::die("String not properly closed: %s", cs);
        res.push_back(token::make(token::string, cs + 1, ptr));
        ptr++;
        continue;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        while (is_digit(*ptr)) ptr++;
        res.push_back(token::make(token::number, cs, ptr));
        continue;
      default:
        if (match(ptr, "INT")) {
          res.push_back(token::kw::INT);
          continue;
        }
        if (match(ptr, "PRINT")) {
          res.push_back(token::kw::PRINT);
          continue;
        }
        if (match(ptr, "SCREEN")) {
          res.push_back(token::kw::SCREEN);
          continue;
        }
        if (is_alpha(c)) {
          while (is_alpha(*ptr) || is_digit(*ptr)) ptr++;
          res.push_back(token::make(token::identifier, cs, ptr));
          continue;
        }
        silog::die("invalid char: [%c]", *ptr);
        continue;
    }
  }
  res.reset();
  g_ts = traits::move(res);
}

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
  tokenise(src);
  while (parse_line()) {}
}

int main(int argc, char ** argv) try {
  jojo::read("example1.bas", nullptr, compile);
  jojo::read("example2.bas", nullptr, compile);
} catch (...) {
  return 1;
}
