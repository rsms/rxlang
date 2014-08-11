#pragma once
#include "join.hh"
#include "hash.hh"

namespace rx {

struct Pkg;
using std::string;

using PkgImports = std::set<Pkg>;
  // Imports need to be ordered and unique -- both properties of std::set :)


struct PkgUnionID {
  // Identifier for a union of packages.
  // E.g. ("foo/a/bar", "lol/cat") -> "hFHz7t9XWzYZa2MhGWHJO0"
  PkgUnionID();
  PkgUnionID(const PkgImports&);
  string toString() const;
private:
  char _s[22];
};


struct Pkg {
  Pkg(string&& name);
  Pkg(const string& name) : Pkg{std::move(string{name})} {}
  Pkg(const char* name) : Pkg{std::move(string{name})} {}

  string::const_iterator cbegin() const { return _name.cbegin(); }
  string::const_iterator cend() const { return _name.cend(); }
  const string& name() const { return _name; } // "foo/a/bar"
  string basename() const; // "bar"

  bool operator==(const string& name) const { return _name == name; }
  int operator<(const Pkg& other) const { return _name < other._name; }
  operator std::string() const { return _name; }

private:
  string _name;
};

// ================================================================================================


inline PkgUnionID::PkgUnionID(const PkgImports& packages) {
  hash::B16 r;
  hash::murmur3_128(join(packages, string{1,'\0'}, [](const Pkg& v) { return v.name(); }), r);
  // Murmur3 has a very good key distribution
  hash::encode_128(r, _s);
}

inline string PkgUnionID::toString() const {
  return string{(const char*)_s, 22};
}

inline Pkg::Pkg(string&& name) : _name{std::forward<decltype(name)>(name)} {
  assert(!_name.empty());
  assert(_name.front() != '/');
  assert(_name.back() != '/');
  // TODO: instead of asserts, clean up string or provide a "pkg name validation" function.
}

inline string Pkg::basename() const {
  assert(_name.back() != '/');
  auto z = _name.rfind('/');
  return (z == string::npos) ? _name : _name.substr(z+1);
}

inline std::ostream& operator<< (std::ostream& os, const Pkg& v) {
  return os << v.name();
}

} // namespace
