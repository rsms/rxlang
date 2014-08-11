#pragma once
#include "func.hh"
#include "once.hh"
namespace rx {


// template <typename T> constexpr T fwdarg(T a) { return std::forward<T>(a); }
#define fwdarg(a) ::std::forward<decltype(a)>(a)


constexpr const char* _cx_basename(const char* dp, const char* p) {
  return *p == '\0' ? dp :
         *p == '/'  ? _cx_basename(p+1, p+1) :
                      _cx_basename(dp, p+1);
}
constexpr const char* cx_basename(const char* s) { return _cx_basename(s,s); }

// Raw memory management

template <typename T> T* alloc(size_t n) {
  assert(n > 0);
  return (T*)::malloc(sizeof(T) * n);
}
template <typename T> T* alloc(std::nullptr_t v, size_t n) {
  assert(n > 0);
  return (T*)::calloc(n, sizeof(T));
}
  // Allocate a chunk of memory for T. Form 2 returns a zeroed chunk while form 1 doesn't.

template <typename T> void dealloc(T*& v) {
  ::free((void*)v); v = nullptr;
}

template <typename T> T* copy(T* a, size_t z) {
  return a ? (T*)::memcpy(::malloc(z), (const void*)a, z) : a;
}

template <typename T> T* copy(T* a) { return copy(a, sizeof(T)); }


// Regex

#define rx_cregex(pat) \
  std::regex{pat, std::regex_constants::ECMAScript|std::regex_constants::optimize}

#define rx_sregex(pat) std::regex{\
  std::string{pat}, std::regex_constants::ECMAScript|std::regex_constants::optimize}

/* Example

auto re = rx_cregex(R"(\.([^\.]+)$)");
std::cmatch m;
if (std::regex_search(ent.c_str(), m, re)) {
  string s{m[1].first, m[1].second};
    // string{string_iterator{const char*},string_iterator{const char*}}
  cerr << "m[1] = '" << s << "'" << endl;
  cerr << "source file: '" << ent << "'" << endl;
}

*/


} // namespace
