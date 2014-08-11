#pragma once
#include "pkg.hh"
#include "util.hh"
#include "error.hh"
#include "text.hh"
namespace rx {
using std::string;

#define RX_LEX_TOKENS(T) \
  /*Name,       HasValue */ \
  T( Error,        0 ) \
  T( End,          0 ) \
  T( LineBreak,    0 ) \
  T( Op,           1 ) \
  T( Solidus,      0 ) \
  T( Eq,           0 ) \
  T( EqEq,         0 ) \
  T( Comma,        0 ) \
  T( Colon,        0 ) \
  T( Semicolon,    0 ) \
  T( OpenAng,      0 ) \
  T( CloseAng,     0 ) \
  T( Dot,          0 ) \
  T( TwoDots,      0 ) \
  T( ThreeDots,    0 ) \
  T( DecIntLit,    1 ) \
  T( OctIntLit,    1 ) \
  T( HexIntLit,    1 ) \
  T( FloatLit,     1 ) \
  T( OpenBlock,    0 ) \
  T( CloseBlock,   0 ) \
  T( OpenGroup,    0 ) \
  T( CloseGroup,   0 ) \
  T( OpenCollLit,  0 ) \
  T( CloseCollLit, 0 ) \
  T( Symbol,       1 ) \
  T( LineComment,  1 ) \
  T( CharLit,      1 ) \
  T( TextLit,      1 ) \


struct Lex {

  enum Token {
    #define T(Name, HasValue) Name,
    RX_LEX_TOKENS(T)
    #undef T
  };

  Lex(const char* p, size_t z);
  ~Lex();
  bool isValid() const;
  const rx::Error& lastError() const;
  Token next(Text&);
  Token current() const;
  static string repr(Token, const Text& value);

private:
  struct Imp; Imp* self = nullptr;
};



} // namespace
