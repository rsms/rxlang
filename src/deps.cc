#include "deps.hh"
#include "time.hh"
namespace rx {

using std::cerr;
using std::endl;

#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)


PkgDeps::PkgDeps(
    const string& rxDir,
    const string& pkgDir,
    const Pkg& pkg,
    ResolveCallback resolveCB)
  : _rxDir{rxDir}
  , _pkgDir{pkgDir}
  , _pkg{pkg}
  , _resolveCB{resolveCB}
{}



static const std::set<string> kSourceFileExts{string{"rx"}, string{"cc"}};


AsyncCanceler PkgDeps::findSrcFilesAtDir(const string& path, func<void(Error,SrcFileSet&&)> cb) {
  SrcFileSet* srcFiles = new SrcFileSet;
  return fs::scandir(
    path,
    Async::main(),
    /*depth=*/0,
    [=](const string& dirname, const string& filename, const fs::Stat& st) mutable {
      auto ext = fs::pathExt(filename);
      if (st.isFile() && kSourceFileExts.find(ext) != kSourceFileExts.end()) {
        srcFiles->emplace(_pkg, st, filename, ext);
      }
      return true;
    },
    [=](Error err) mutable {
      cb(err, SrcFileSet{std::move(*srcFiles)});
      delete srcFiles;
    }
  );
}


AsyncCanceler PkgDeps::processSrcFiles(func<void(Error)> cb) {
  AsyncGroup asyncGroup{cb};

  auto I = _srcFiles.begin();
  auto E = _srcFiles.end();

  for (; I != E; ++I) {
    SrcFile* srcFile = const_cast<SrcFile*>(&*I); // WTF?! Should be non-const already, right?
    DBG("  - '" << srcFile->filename() <<
        "' at '" << fs::pathJoin(_rxDir, "src", srcFile->pathname()) << "'");

    auto* job = asyncGroup.begin();
    *job = fs::readfile(
      Async::main(),
      fs::pathJoin(_rxDir, "src", srcFile->pathname()),
      srcFile->stat().size,
      [=](Error err, fs::FileData&& d) {
        if (!err) {
          srcFile->setData(std::move(d));
          err = srcFile->parse();
        }
        asyncGroup.end(job, err);
      }
    );
  }

  // asyncGroup.canceler()(); return nullptr;
  return asyncGroup.canceler();
}


void PkgDeps::resolve() {
  DBG("Resolving package " << _pkg)

  DBG("srcDir():     '" << srcDir() << "'")
  DBG("binFile():    '" << binFile() << "'")
  DBG("pkgPCHFile(): '" << pkgPCHFile() << "'")
  DBG("pkgObjFile(): '" << pkgObjFile() << "'")

  DBG("Locating source files for package " << _pkg)
  findSrcFilesAtDir(srcDir(), [=](Error err, SrcFileSet&& srcFiles) {
    if (err) { _resolveCB(err); return; }
    _srcFiles = std::move(srcFiles);
    DBG("Processing source files for package " << _pkg)
    processSrcFiles([](Error err) {
      DBG("ProcessSrcFiles completed. err=" << err)
    });
  });
  
}


} // namespace
