#include "test.hh"
#include "text.hh"
#include "text.def"

int main(int argc, const char** argv) {
  using namespace rx::text;

  // Verify that all codepoints listed in RX_TEXT_INVALID_RANGES are categorized as Unassigned

  #define A_UNASSIGNED_CP(cp)  A(category(cp) == Category::Unassigned);

  #define CP(cp) A_UNASSIGNED_CP(cp)
  #define CR(StartC, EndC) { \
    uint32_t cp = StartC; \
    do { A_UNASSIGNED_CP(cp) } while (cp++ != EndC); \
  }

  RX_TEXT_INVALID_RANGES(CP,CR)

  #undef CP
  #undef CR

  return 0;
}
