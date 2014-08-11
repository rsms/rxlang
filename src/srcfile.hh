#pragma once
#include "pkg.hh"
#include "util.hh"
#include "error.hh"
#include "fs.hh"
namespace rx {
using std::string;


struct SrcFile {
  SrcFile(const Pkg&, const fs::Stat&, const string& filename, const string& nameext);

  const Pkg&          pkg() const;      // Package it belongs to
  const fs::Stat&     stat() const;
  const string&       filename() const; // e.g. "bar.cc" or "bar.rx"
  const string&       nameext() const;  // e.g. "cc" or "rx"
  const string&       pathname() const; // e.g. "bar/bar.cc" or "foo/bar/bar.rx"
  const fs::FileData& data() const;
  void setData(fs::FileData&&);

  Error parse();

  bool operator<(const SrcFile& other) const { return _pathname > other._pathname; } // for b-trees
  // bool operator<(const SrcFile& other) const { return stat.ino < other.stat.ino; } // for b-trees
  // bool operator==(const SrcFile& other) const { return pathname == other.pathname; } // for hashes
  // struct Hash { size_t operator()(const SrcFile& v) const {
  //   return std::hash<string>()(v.pathname); } };

private:
  Pkg          _pkg;      // Package it belongs to
  fs::Stat     _stat;
  string       _filename; // e.g. "bar.cc" or "bar.rx"
  string       _nameext;  // e.g. "cc" or "rx"
  string       _pathname; // e.g. "bar/bar.cc" or "foo/bar/bar.rx"
  fs::FileData _data;
};

using SrcFileSet = std::set<SrcFile>;

// ================================================================================================

inline SrcFile::SrcFile(
    const Pkg& pkg,
    const fs::Stat& st,
    const string& filename,
    const string& nameext)
  : _pkg{pkg}
  , _stat{st}
  , _filename{filename}
  , _nameext{nameext}
  , _pathname{fs::pathJoin(pkg.name(), filename)}
{}

inline const Pkg&          SrcFile::pkg() const { return _pkg; }
inline const fs::Stat&     SrcFile::stat() const { return _stat; }
inline const string&       SrcFile::filename() const { return _filename; }
inline const string&       SrcFile::nameext() const { return _nameext; }
inline const string&       SrcFile::pathname() const { return _pathname; }
inline const fs::FileData& SrcFile::data() const { return _data; }
inline void SrcFile::setData(fs::FileData&& data) { _data = fwdarg(data); }


} // namespace
