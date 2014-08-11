#pragma once
#include "pkg.hh"
#include "util.hh"
#include "error.hh"
#include "fs.hh"
#include "asyncgroup.hh"
#include "srcfile.hh"
namespace rx {
using std::string;

struct PkgDeps {
  // Represents dependencies and up-to-date status of a package
  using ResolveCallback = func<void(Error)>;
  PkgDeps(
    const string& rxDir,
    const string& pkgDir,
    const Pkg&,
    ResolveCallback);

  string srcDir() const;     // "~/rx/src/foo/bar"
  string binFile() const;    // "~/rx/bin/bar"
  string pkgPCHFile() const; // "~/rx/pkg/target/foo/bar.pch"
  string pkgObjFile() const; // "~/rx/pkg/target/foo/bar.a"

  void resolve();

  AsyncCanceler findSrcFilesAtDir(const string& path, func<void(Error,SrcFileSet&&)>);
  AsyncCanceler processSrcFiles(func<void(Error)>);
  Error parseSrcFile(SrcFile&);

private:
  string          _rxDir;
  string          _pkgDir;
  string          _srcFilename;
  Pkg             _pkg;
  ResolveCallback _resolveCB;

  SrcFileSet      _srcFiles;
  PkgImports      _imports;
};

inline string PkgDeps::srcDir() const {     return  _rxDir + "/src/" + _pkg.name(); }
inline string PkgDeps::binFile() const {    return  _rxDir + "/bin/" + _pkg.basename(); }
inline string PkgDeps::pkgPCHFile() const { return _pkgDir + "/" + _pkg.name() + ".pch"; }
inline string PkgDeps::pkgObjFile() const { return _pkgDir + "/" + _pkg.name() + ".a"; }

} // namespace
