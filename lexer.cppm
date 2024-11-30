export module lexer;
import jute;
import hai;
import silog;

export namespace token {
  enum type {
    nil,
    number,
    string,
    keyword,
    identifier,
    symbol,
    oper,
    parenthesis,
    newline,
    eof,
  };

  struct t {
    type type;
    jute::view content;
    jute::view file;
    int line;
    int column;
  };
  constexpr bool operator==(const token::t & a, const token::t & b) {
    return a.type == b.type && a.content == b.content;
  }
  [[noreturn]] void fail(const char * msg, token::t t) {
    silog::die("%.*s:%d:%d %s",
        static_cast<unsigned>(t.file.size()), t.file.data(),
        t.line, t.column, msg);
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
}
export namespace token::kw {
  constexpr t INT    { .type = keyword, .content = "INT" };
  constexpr t PRINT  { .type = keyword, .content = "PRINT" };
  constexpr t PSET   { .type = keyword, .content = "PSET" };
  constexpr t RND    { .type = keyword, .content = "RND" };
  constexpr t SCREEN { .type = keyword, .content = "SCREEN" };
}
export namespace token::sym {
  constexpr t EQ    { .type = symbol, .content = "=" };
  constexpr t COMMA { .type = symbol, .content = "," };
}
export namespace token::paren {
  constexpr t L { .type = parenthesis, .content = "(" };
  constexpr t R { .type = parenthesis, .content = ")" };
}

static token::t make(token::type tp, const char * start, const char * end) {
  unsigned sz = end - start;
  return { .type = tp, .content = { start, sz } };
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

export auto tokenise(jute::view fname, hai::cstr & src) {
  token::stream res {};
  const char * ptr = src.begin();
  const char * line_ptr = src.begin();
  int line { 1 };

  const auto push = [&](token::t t) {
    t.file = fname;
    t.line = line;
    t.column = ptr - line_ptr + 1;
    res.push_back(t);
  };

  while (auto c = *ptr) {
    auto cs = ptr;
    switch (c) {
      case ' ': ptr++; continue;
      case '(': push(token::paren::L); ptr++; continue;
      case ')': push(token::paren::R); ptr++; continue;
      case '=': push(token::sym::EQ); ptr++; continue;
      case ',': push(token::sym::COMMA); ptr++; continue;
      case '\n':
        push(make(token::newline, cs, ++ptr));
        line++;
        line_ptr = ptr;
        continue;
      case '+': case '-': case '*': case '/':
        push(make(token::oper, cs, ++ptr));
        continue;
      case '"':
        do { ptr++; } while (*ptr && *ptr != '"');
        if (!*ptr) silog::die("String not properly closed: %s", cs);
        push(make(token::string, cs + 1, ptr));
        ptr++;
        continue;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        while (is_digit(*ptr)) ptr++;
        push(make(token::number, cs, ptr));
        continue;
      default:
        if (match(ptr, "INT")) {
          push(token::kw::INT);
          continue;
        }
        if (match(ptr, "PRINT")) {
          push(token::kw::PRINT);
          continue;
        }
        if (match(ptr, "PSET")) {
          push(token::kw::PSET);
          continue;
        }
        if (match(ptr, "RND")) {
          push(token::kw::RND);
          continue;
        }
        if (match(ptr, "SCREEN")) {
          push(token::kw::SCREEN);
          continue;
        }
        if (is_alpha(c)) {
          while (is_alpha(*ptr) || is_digit(*ptr)) ptr++;
          push(make(token::identifier, cs, ptr));
          continue;
        }
        silog::die("invalid char: [%c]", *ptr);
        continue;
    }
  }
  res.reset();
  return res;
}

