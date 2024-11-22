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
    op,
    newline,
    eof,
  };
  struct t {
    type type;
    jute::view content;
  };
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
  };

  static t make(type tp, const char * start, const char * end) {
    unsigned sz = end - start;
    return { .type = tp, .content = { start, sz } };
  }
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

static token::stream tokenise(hai::cstr & src) {
  token::stream res {};
  const char * ptr = src.begin();
  while (auto c = *ptr) {
    auto cs = ptr;
    switch (c) {
      case ' ': ptr++; continue;
      case '\n':
        res.push_back(token::make(token::newline, cs, ++ptr));
        continue;
      case '=':
      case ',':
      case '(': case ')':
      case '+': case '-': case '*': case '/':
        res.push_back(token::make(token::op, cs, ++ptr));
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
        if (match(ptr, "PRINT")) {
          res.push_back(token::t { token::keyword, "PRINT" });
          continue;
        }
        if (match(ptr, "SCREEN")) {
          res.push_back(token::t { token::keyword, "SCREEN" });
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
  return res;
}

static void do_print(token::stream & ts) {
  auto t = ts.peek();
  if (t.type == token::string) {
    ts.take();
  } else if (t.type == token::number) {
    ts.take();
  } else if (t.type == token::identifier) {
    ts.take();
  } else if (t.type == token::newline) {
  } else silog::die("can't print token %s", t.content.cstr().begin());
}

static void do_screen(token::stream & ts) {
  auto t = ts.take();
  if (t.type == token::number) {
  } else silog::die("invalid screen mode %s", t.content.cstr().begin());
}

static void do_assign(token::stream & ts) {
  auto t = ts.take();
  if (t.type == token::op && t.content == "=") {
    silog::die("TBD");
  } else silog::die("not sure what to do with %s", t.content.cstr().begin());
}

static bool parse_line(token::stream & ts) {
  auto l_num = ts.take();
  if (l_num.type == token::eof) return false;
  if (l_num.type != token::number) silog::die("line starting without a number");
  while (ts.peek().type != token::newline) {
    auto t = ts.take();
    switch (t.type) {
      case token::identifier:
        do_assign(ts);
        break;
      case token::keyword:
        if (t.content == "PRINT") do_print(ts);
        else if (t.content == "SCREEN") do_screen(ts);
        else silog::die("unexpected keyword at line %s", l_num.content.cstr().begin());
        break;
      default: silog::die("unexpected token type at line %s", l_num.content.cstr().begin());
    }
  }
  silog::trace(l_num.content);
  return ts.take().type != token::eof;
}

static void compile(void *, hai::cstr & src) {
  auto tokens = tokenise(src);
  while (parse_line(tokens)) {}
}

int main(int argc, char ** argv) try {
  jojo::read("example1.bas", nullptr, compile);
  jojo::read("example2.bas", nullptr, compile);
} catch (...) {
  return 1;
}
