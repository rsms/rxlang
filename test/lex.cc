#include "test.hh"
#include "lex.hh"
#include "fs.hh"

using std::cerr;
using std::endl;
using std::string;
using namespace rx;

int main(int argc, const char** argv) {

  #define A_Tok(T) do { \
    A(lex.isValid()); \
    auto tok = lex.next(value); \
    if (tok != T) { \
      cerr << "Asserion failure: Expected `lex.next(value)` to return `" #T "` " \
           << "but got `" \
           << (tok == Lex::Error ? string{"Error: "} + lex.lastError().message() \
                                 : Lex::repr(tok, value)) \
           << "` at line " << __LINE__ << endl; \
      exit(1); \
    } \
  } while(0);

  #define A_End A(!lex.isValid());

  #define A_Val(V)    A(value == V);
  #define A_TokV(T,V)  A_Tok(T) A_Val(V)
  
  #define A_Line      A_Tok('\n')
  #define A_SemiLine \
    A(lex.next(value) == ';'); A_Line

  #define A_SrcFails(SRC) { \
    const char* src = SRC; \
    Lex lex{src, strlen(src)}; Text value; \
    A_Tok(Lex::Error) \
  }

  { // ==== Integer literals ====
    const char* src =
      "0 42\n"
      "0600\n"
      "0xBadFace\n"
      "170141183460469231731687303715884105727\n"
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_TokV(Lex::DecIntLit, U"0")  A_TokV(Lex::DecIntLit, U"42")  A_SemiLine
    A_TokV(Lex::OctIntLit, U"600");  A_SemiLine
    A_TokV(Lex::HexIntLit, U"BadFace");  A_SemiLine
    A_TokV(Lex::DecIntLit, U"170141183460469231731687303715884105727");  A_SemiLine
    A_End
  }

  { // ==== Float literals ====
    const char* src =
      "0.\n"
      "72.40\n"
      "072.40\n"
      "2.71828\n"
      "1.e+0\n"
      "6.67428e-11\n"
      "1E6\n"
      ".25\n"
      ".12345E+5\n"
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_TokV(Lex::FloatLit, U"0.")            A_SemiLine
    A_TokV(Lex::FloatLit, U"72.40")         A_SemiLine
    A_TokV(Lex::FloatLit, U"072.40")        A_SemiLine
    A_TokV(Lex::FloatLit, U"2.71828")       A_SemiLine
    A_TokV(Lex::FloatLit, U"1.e+0")         A_SemiLine
    A_TokV(Lex::FloatLit, U"6.67428e-11")   A_SemiLine
    A_TokV(Lex::FloatLit, U"1E6")           A_SemiLine
    A_TokV(Lex::FloatLit, U".25")           A_SemiLine
    A_TokV(Lex::FloatLit, U".12345E+5")     A_SemiLine
    A_End
  }

  { // ==== Atoms ====
    const char* src =
      "\n.;..;...:(){}[]<>/,=="
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_Tok('\n')
    A_Tok('.')  A_Tok(';')
    A_Tok(Lex::DotDot)  A_Tok(';')
    A_Tok(Lex::DotDotDot)
    A_Tok(':')
    A_Tok('(')  A_Tok(')')
    A_Tok('{')  A_Tok('}')
    A_Tok('[')  A_Tok(']')
    A_Tok('<')  A_Tok('>')
    A_Tok('/')
    A_Tok(',')
    A_Tok(Lex::EqEq)
    A_End
  }

  A_SrcFails("....")  // More than three dots in a sequence

  { // ==== Comments ====
    const char* src =
      "0\n"
      "//A\n"
      "1//B // x\n"
      "2 // C 3"
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_TokV(Lex::DecIntLit, U"0")  A_SemiLine
    A_TokV(Lex::LineComment, U"A")  A_Line
    A_TokV(Lex::DecIntLit, U"1")  A_TokV(Lex::LineComment, U"B // x") A_Line
    A_TokV(Lex::DecIntLit, U"2")  A_TokV(Lex::LineComment, U" C 3")
    A_End
  }

  #define A_Sym(V)  A_TokV(Lex::Symbol, V)

  { // ==== Symbols ====
    const char* src =
      "a bc d;e.d\n"
      "\xF0\x9F\x98\xB0\n" // U+1F630 "😰"
      "\xE6\x97\xA5" "\xE6\x9C\xAC" "\xE8\xAA\x9E" "\n" // U+65E5 U+672C U+8A9E "日本語"
      "\xC3\xBF\n" // "ÿ"
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_Sym(U"a")  A_Sym(U"bc")  A_Sym(U"d")  A_Tok(';')
    A_Sym(U"e")  A_Tok('.')  A_Sym(U"d")  A_SemiLine
    A_Sym(U"\U0001F630")  A_SemiLine
    A_Sym(U"\u65E5\u672C\u8A9E")  A_SemiLine
    A_Sym(U"\u00FF")  A_SemiLine
    A_End
  }

  #define A_Chr(V)  A_TokV(Lex::CharLit, V)

  { // ==== Char literals ====
    const char* src =
      "'a'\n"
      "'b''c''d'\n"
      "'\\''\n" // '\''
      "'\\U0001F630' '\xF0\x9F\x98\xB0'\n" // U+1F630 '😰'
      "'\xC3\xBF' '\\xff' '\\u00FF'\n"     // 'ÿ'
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_Chr(U"a")  A_SemiLine
    A_Chr(U"b")  A_Chr(U"c")  A_Chr(U"d")  A_SemiLine
    A_Chr(U"'")  A_SemiLine
    A_Chr(U"\U0001F630")  A_Chr(U"\U0001F630")  A_SemiLine
    A_Chr(U"\u00FF")  A_Chr(U"\u00FF")  A_Chr(U"\u00FF")  A_SemiLine
    A_End
  }

  A_SrcFails("\\\"")  // Escaping a text delimiter

  #define A_Txt(V)  A_TokV(Lex::TextLit, V)

  { // ==== Text literals ====
    const char* src =
      "\"a\"\n"
      "\"bc d\"\"ef\"\"g\"\n"
      "\"\\\"\"\n" // "\""
      "\"\\U0001F630\" \"\xF0\x9F\x98\xB0\"\n" // U+1F630 "😰"
      "\"\xC3\xBF\\xff\\u00FF\"\n"     // "ÿÿÿ"
      "\"" "\xE6\x97\xA5" "\xE6\x9C\xAC" "\xE8\xAA\x9E" "\"" "\n" // U+65E5 U+672C U+8A9E "日本語"
      "\"\\u65e5\xE6\x9C\xAC\\U00008a9e\"\n"  // U+65E5 U+672C U+8A9E "\u65e5本\U00008a9e"
    ;
    Lex lex{src, strlen(src)}; Text value;
    A_Txt(U"a")  A_SemiLine
    A_Txt(U"bc d")  A_Txt(U"ef")  A_Txt(U"g")  A_SemiLine
    A_Txt(U"\"")  A_SemiLine
    A_Txt(U"\U0001F630")  A_Txt(U"\U0001F630")  A_SemiLine
    A_Txt(U"\u00FF\u00FF\u00FF")  A_SemiLine
    A_Txt(U"\u65E5\u672C\u8A9E")  A_SemiLine
    A_Txt(U"\u65E5\u672C\u8A9E")  A_SemiLine
    A_End
  }

  A_SrcFails("\"\\'\"")  // Escaping a char delimiter
  A_SrcFails("\"\\u65e5\xE6\x9C\xAC\\U00008a9e")  // Unterminated literal at end of input
  A_SrcFails("\"foo\nbar\"")  // Linebreak

  return 0;
}

// void loadSrcFile(const string& filename, fs::FileData& src) {
//   auto err = fs::readfile(filename, src);
//   if (err) { cerr << err << endl; exit(1); }
// }
