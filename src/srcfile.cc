#include "srcfile.hh"
#include "lex.hh"
namespace rx {

using std::cerr;
using std::endl;

#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)


Error SrcFile::parse() {

  if (_nameext == "rx") {
    Lex lex{_data.data(), _data.size()};
    Text tokValue;

    while (lex.isValid()) {
      auto tok = lex.next(tokValue);
      switch (tok) {
        case Lex::Error: return lex.lastError();
        case Lex::End:   break;
        default: {
          cerr << Lex::repr(tok, tokValue)
               << "  @ "
               << "offset:" << lex.srcLocation().offset << ", "
               << "length:" << lex.srcLocation().length << ", "
               << "line:  " << lex.srcLocation().line << ", "
               << "column:" << lex.srcLocation().column
               << endl;
          break;
        }
      }
    }
  }

  // DBG("parse " << pathname())
  // DBG("  file[0] = '" << (char)_data[0] << "'")
  // DBG("  file[1] = '" << (char)_data[1] << "'")
  // DBG("  file[2] = '" << (char)_data[2] << "'")
  return nullptr;
}


} // namespace
