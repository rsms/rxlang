#include "test.hh"
#include "text.hh"

int main() {
  using std::string;
  using rx::Text;
  using namespace rx::text;
  A(isValidChar('A') == true);

  Text utf32 = decodeUTF8("\xF0\x9F\x98\x84");
  A(utf32.size() == 1);
  A(utf32[0] == 0x1F604);

  string utf8 = encodeUTF8(utf32);
  A(utf8.size() == 4);
  A(utf8[0] == '\xF0');
  A(utf8[1] == '\x9F');
  A(utf8[2] == '\x98');
  A(utf8[3] == '\x84');

  return 0;
}