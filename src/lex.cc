#include "lex.hh"

#if !defined(__EXCEPTIONS) || !__EXCEPTIONS
  #include "utf8/unchecked.h"
  namespace UTF8 = utf8::unchecked;
#else
  #include "utf8/checked.h"
  namespace UTF8 = utf8;
#endif


namespace rx {

using std::cerr;
using std::endl;
#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)

#define FOREACH_CHAR \
  while (_p != _end) switch (nextChar())


#include "text.def"

#define LINEBREAK_CP(cp, name) case cp:
#define LINEBREAK_CASES  RX_TEXT_LINEBREAK_CHARS(LINEBREAK_CP)

#define WHITESPACE_CP(cp, name) case cp:
#define WHITESPACE_CASES  RX_TEXT_WHITESPACE_CHARS(WHITESPACE_CP)

#define CTRL_CP(cp, name) case cp:
#define CTRL_CASES  RX_TEXT_CTRL_CHARS(CTRL_CP)

#define OCTNUM_CP(cp, name) case cp:
#define OCTNUM_CASES  RX_TEXT_OCTDIGIT_CHARS(OCTNUM_CP)

#define DECNUM_CP(cp, name) case cp:
#define DECNUM_CASES  RX_TEXT_DECDIGIT_CHARS(DECNUM_CP)

#define HEXNUM_CP(cp, name) case cp:
#define HEXNUM_CASES  RX_TEXT_HEXDIGIT_CHARS(HEXNUM_CP)

#define XXXXXSUBLIMETEXTWTFS


struct Lex::Imp {
  const char* _begin;
  const char* _end;
  const char* _p;
  UChar       _c;  // current character
  Token       _tok;
  bool        _tokIsQueued = false;
  const char* _lineBegin;
  SrcLocation _srcLoc;
  rx::Error   _err;

  Imp(const char* p, size_t z) : _begin{p}, _end{p+z}, _p{p}, _tok{Tokens::End} {}


  void undoChar() {
    _p -= text::UTF8SizeOf(_c);
  }

  UChar nextChar() {
    _c = UTF8::next(_p, _end);
    return _c;
  }

  Token setTok(Token t) {
    _srcLoc.length = (_p - _begin) - _srcLoc.offset;
    return _tok = t;
  }

  Token error(const string& msg) {
    _err = {msg};
    return setTok(Error);
  }


  Token next(Text& value) {
    if (_tokIsQueued) {
      // Return queued token
      _tokIsQueued = false;
      return _tok;
    }

    if (_tok == '\n') {
      // Increase line if previous token was a linebreak. This way linebreaks are semantically
      // at the end of a line, rather than at the beginning of a line.
      ++_srcLoc.line;
      _lineBegin = _p;
    }

    _srcLoc.offset = _p - _begin;
    _srcLoc.column = _p - _lineBegin;
    value.clear();
      // Set source location and clear value

    // The root switch has a dual purpose: Initiate tokens and reading symbols.
    // Because symbols are pretty much "anything else", this is the most straight-forward way.
    bool isReadingSym = false;
    #define ADDSYM_OR if (isReadingSym) { value += _c; break; } else
    #define ENDSYM_OR if (isReadingSym) { undoChar(); return setTok(Symbol); } else

    FOREACH_CHAR {
      CTRL_CASES  WHITESPACE_CASES  ENDSYM_OR break; // ignore

      case '\n':
        ENDSYM_OR {
          // When the input is broken into tokens, a semicolon is automatically inserted into the
          // token stream at the end of a non-blank line if the line's final token is
          //   • an identifier
          //   • an integer, floating-point, imaginary, rune, or string literal
          //   • one of the keywords break, continue, fallthrough, or return
          //   • one of the operators and delimiters ++, --, ), ], or }
          if ( _tok == Symbol ||
              (_tok > BeginLit && _tok < EndLit) ||
              _tok == U')' ||
              _tok == U']' ||
              _tok == U'}'
          ) {
            setTok('\n');
            _tokIsQueued = true;
            return ';';
          } else {
            setTok('\n');
            return _tok;
          }
        }

      case '{':  case '}':
      case '(':  case ')':
      case '[':  case ']':
      case '<': case '>':
      case ':': case ',': case ';':
      case '+': case '-': case '*':
        ENDSYM_OR return setTok(_c);

      case '/':  ENDSYM_OR return readSolidus(value);
      case '=':  ENDSYM_OR return readEq(value);

      case '.':  ENDSYM_OR return readDot(value); // "." | ".." | "..." | "."<float>

      case '0':         ADDSYM_OR return readZeroLeadingNumLit(value);
      case '1' ... '9': ADDSYM_OR return readDecIntLit(value);

      case '\'':  return readCharLit(value);
      case '"':   return readTextLit(value);

      default: {
        if (text::isValidChar(_c)) {
          isReadingSym = true;
          value += _c;
        } else {
          return error("Illegal character "+text::repr(_c)+" in input");
        }
        break;
      }
    }

    return isReadingSym ? setTok(Symbol) : Lex::End;
  }


  Token readEq(Text& value) {
    // Eq   = "=" | EqEq
    // EqEq = "=="
    switch (nextChar()) {
      case UCharMax:        return error("Unexpected end of input");
      case '=':             return setTok(EqEq);
      default:  undoChar(); return setTok(U'=');
    }
  }


  Token readSolidus(Text& value) {
    // Solidus     = "/" | LineComment
    // LineComment = "//" <any except LF> <LF>
    switch (nextChar()) {
      case UCharMax:        return error("Unexpected end of input");
      case '/':             return readLineComment(value);
      default:  undoChar(); return setTok(U'/');
    }
  }


  Token readLineComment(Text& value) {
    // Enter at "//"
    FOREACH_CHAR {
      case '\n': undoChar(); return setTok(LineComment);
      default:   value += _c; break;
    }
    return setTok(LineComment);
  }


  template <UChar TermC>
  bool readCharLitEscape(Text& value) {
    // EscapedUnicodeChar  = "\n" | "\r" | "\t" | "\\" | <TermC>
    //                       | LittleXUnicodeValue
    //                       | LittleUUnicodeValue
    //                       | BigUUnicodeValue
    // LittleXUnicodeValue = "\x" HexDigit HexDigit
    // LittleUUnicodeValue = "\u" HexDigit HexDigit HexDigit HexDigit
    // BigUUnicodeValue    = "\U" HexDigit HexDigit HexDigit HexDigit
    //                            HexDigit HexDigit HexDigit HexDigit
    switch (nextChar()) {
      case 'n': { value += '\n'; break; }
      case 'r': { value += '\r'; break; }
      case 't': { value += '\t'; break; }
      case TermC:
      case '\\': { value += _c; break; }
      case 'x': { return readHexUChar(2, value); }
      case 'u': { return readHexUChar(4, value); }
      case 'U': { return readHexUChar(8, value); }
      default: {
        error("Unexpected escape sequence '\\' "+text::repr(_c)+" in character literal");
        return false;
      }
    }
    return true;
  }


  Token readCharLit(Text& value) {
    // CharLit = "'" ( UnicodeChar | EscapedUnicodeChar<'> ) "'"
    switch (nextChar()) {
      case UCharMax:  return error("Unterminated character literal at end of input");
      LINEBREAK_CASES return error("Illegal character in character literal");
      case '\'':      return error("Empty character literal or unescaped ' in character literal");
      case '\\':      if (!readCharLitEscape<'\''>(value)) return _tok; break;
      default:        value = _c; break;
    }
    switch (nextChar()) {
      case '\'':      return setTok(CharLit);
      default:        return error("Expected ' to end single-character literal");
    }
  }


  Token readTextLit(Text& value) {
    // TextLit = '"' ( UnicodeChar | EscapedUnicodeChar<"> )* '"'
    FOREACH_CHAR {
      LINEBREAK_CASES return error("Illegal character in character literal");
      case '"':       return setTok(TextLit);
      case '\\':      if (!readCharLitEscape<'"'>(value)) return _tok; break;
      default:        value += _c; break;
    }
    return error("Unterminated character literal at end of input");
  }


  bool readHexUChar(int nbytes, Text& value) {
    string s;
    s.reserve(nbytes);
    int i = 0;
    while (i++ != nbytes) {
      switch (nextChar()) {
        case '0' ... '9': case 'A' ... 'F': case 'a' ... 'f': {
          s += (char)_c; break;
        }
        default: {
          error("Invalid Unicode sequence");
          return false;
        }
      }
    }
    UChar c = std::stoul(s, 0, 16);
    switch (text::category(c)) {
      case text::Category::Unassigned:
      case text::Category::NormativeCs: {
        error(
          string{"Invalid Unicode character \\"} + (nbytes == 4 ? "u" : "U") + s
        );
        return false;
      }
      default: break;
    }
    value += c;
    return true;
  }


  // === Integer literals ===
  // int_lit     = decimal_lit | octal_lit | hex_lit
  // decimal_lit = ("0" | ( "1" … "9" ) { decimal_digit })
  // octal_lit   = "0" { octal_digit }
  // hex_lit     = "0" ( "x" | "X" ) hex_digit { hex_digit }
  //

  Token readZeroLeadingNumLit(Text& value) {
    value = _c; // enter at _c='0'
    FOREACH_CHAR {
      case 'X': case 'x': return readHexIntLit(value);
      case '.':           return readFloatLitAtDot(value);
      OCTNUM_CASES        return readOctIntLit(value);
      default: {
        undoChar();
        return setTok(DecIntLit); // special case '0'
      }
    }
    return setTok(DecIntLit); // special case '0' as last char in input
  }


  Token readOctIntLit(Text& value) {
    value = _c; // Enter at ('1' ... '7')
    FOREACH_CHAR {
      OCTNUM_CASES { value += _c; break; }
      case '.': {
        value.insert(0, 1, '0');
        return readFloatLitAtDot(value);
      }
      default: { undoChar(); return setTok(OctIntLit); }
    }
    return setTok(OctIntLit);
  }


  Token readDecIntLit(Text& value) {
    value = _c; // Enter at ('1' ... '9')
    FOREACH_CHAR {
      DECNUM_CASES { value += _c; break; }
      case 'e': case 'E':    return readFloatLitAtExp(value);
      case '.':              return readFloatLitAtDot(value);
      default: { undoChar(); return setTok(DecIntLit); }
    }
    return setTok(DecIntLit);
  }


  Token readHexIntLit(Text& value) {
    value.clear(); // ditch '0'. Enter at "x" in "0xN..."
    FOREACH_CHAR {
      HEXNUM_CASES { value += _c; break; }
      default: { undoChar(); return setTok(HexIntLit); }
    }
    return error("Incomplete hex literal"); // special case: '0x' is the last of input
  }


  Token readDot(Text& value) {
    value = _c; // enter at "."
    switch (nextChar()) {
      case UCharMax: return error("Unexpected '.' at end of input");
      case U'.': {
        // Dots      = DotDot | DotDotDot
        // DotDot   = ".."
        // DotDotDot = "..."
        if (nextChar() == U'.') {
          if (nextChar() == U'.') {
            return error("Unexpected '.' after '...'");
          } else {
            undoChar();
            return setTok(DotDotDot);
          }
        } else {
          undoChar();
          return setTok(DotDot);
        }
      }
      DECNUM_CASES         return readFloatLitAtDot(value);
      default: undoChar(); return setTok(U'.');
    }
  }


  Token readFloatLitAtDot(Text& value) {
    value += _c; // else enter at "<decnum>."
    // float_lit = decimals "." [ decimals ] [ exponent ] |
    //             decimals exponent |
    //             "." decimals [ exponent ]
    // decimals  = decimal_digit { decimal_digit }
    // exponent  = ( "e" | "E" ) [ "+" | "-" ] decimals
    FOREACH_CHAR {
      DECNUM_CASES { value += _c; break; }
      case 'e': case 'E':    return readFloatLitAtExp(value);
      default: { undoChar(); return setTok(FloatLit); }
    }
    return setTok(FloatLit); // special case '0.'
  }


  Token readFloatLitAtExp(Text& value) {
    value += _c; // enter at "<decnum>[E|e]"
    if (_p == _end) return error("Incomplete float exponent");
    switch (nextChar()) {
      case '+': case '-': { value += _c; break; }
      DECNUM_CASES { value += _c; break; }
      default: goto illegal_exp_value;
    }

    if (_c == '+' || _c == '-') {
      if (_p == _end) return error("Incomplete float exponent");
      switch (nextChar()) {
        DECNUM_CASES { value += _c; break; }
        default: { goto illegal_exp_value; }
      }
    }

    FOREACH_CHAR {
      DECNUM_CASES { value += _c; break; }
      default: { undoChar(); return setTok(FloatLit); }
    }

    return setTok(FloatLit); // special case '<decnum>[E|e]?[+|-]?<decnum>'

   illegal_exp_value:
    error("Illegal value '" + text::repr(_c) + "' for exponent in float literal");
    undoChar();
    return _tok;
  }


};


Lex::Lex(const char* p, size_t z) : self{new Imp{p, z}} {}
Lex::~Lex() { if (self != nullptr) delete self; }

bool Lex::isValid() const { return self->_p < self->_end || self->_tokIsQueued; }
Lex::Token Lex::current() const { return self->_tok; }
const rx::Error& Lex::lastError() const { return self->_err; }
Lex::Token Lex::next(Text& value) { return self->next(value); }
const Lex::SrcLocation& Lex::srcLocation() const { return self->_srcLoc; }

string Lex::repr(Token t, const Text& value) {
  switch (t) {
    #define T(Name, HasValue) case Lex::Tokens::Name: { \
      return (HasValue && !value.empty()) ? string{#Name} + " \"" + text::repr(value) + "\"" : \
             string{#Name}; \
    }
    RX_LEX_TOKENS(T)
    #undef T
    default:
      return text::repr(t);
  }
}

} // namespace
