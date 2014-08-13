#pragma once
#include "pkg.hh"
#include "util.hh"
#include "error.hh"
#include "text.hh"
namespace rx {
using std::string;

#define RX_LEX_TOKENS(T) \
  /*Name,    0=no value, 1=has value, (=group start, )=group end */ \
  T( Error,            0   ) \
  T( End,              0   ) \
  T( EqEq,             0   ) \
  T( DotDot,           0   ) \
  T( DotDotDot,        0   ) \
  T( BeginLit, '('         ) \
    T( BeginNumLit, '('    ) \
      T( DecIntLit,    1   ) \
      T( OctIntLit,    1   ) \
      T( HexIntLit,    1   ) \
      T( FloatLit,     1   ) \
    T( EndNumLit, ')'      ) \
    T( CharLit,        1   ) \
    T( TextLit,        1   ) \
  T( EndLit, ')'           ) \
  T( Symbol,           1   ) \
  T( LineComment,      1   ) \

struct Lex {
  using Token = UChar;
  Lex(const char* p, size_t z);
  ~Lex();
  bool isValid() const;
  const rx::Error& lastError() const;
  Token next(Text&);
  Token current() const;

  struct SrcLocation {
    size_t   offset = 0; // byte offset into source `p`
    uint32_t length = 0; // number of source bytes
    uint32_t line   = 0; // zero-based
    uint32_t column = 0; // zero-based and expressed in source bytes (not Unicode characters)
  };
  const SrcLocation& srcLocation() const;
    // Source location of current token 

  static string repr(Token, const Text& value);

  enum Tokens : Token {
    BeginSpecialTokens = 0xFFFFFF, // way past last valid Unicode point
    #define T(Name, HasValue) Name,
    RX_LEX_TOKENS(T)
    #undef T
  };

private:
  struct Imp; Imp* self = nullptr;
};



} // namespace
