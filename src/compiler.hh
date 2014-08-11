#pragma once
#include "pkg.hh"
// #include "clang/Frontend/FrontendAction.h"
// #include "clang/Frontend/FrontendOptions.h"
// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"

namespace rx {

using std::string;


struct Compiler final {
  Compiler();
  ~Compiler();

  const string& rxDir() const;
  const string& pkgDir() const;

  string srcPathForPkg(const Pkg&); // e.g. rxdir/src/foo/a/bar
  string binPathForPkg(const Pkg&); // e.g. rxdir/pkg/target-config/foo/a/bar
  string PCHPathForPkg(const Pkg&); // e.g. rxdir/pkg/target-config/foo/a/bar.pch
  string PCHPathForPkgUnionID(const PkgUnionID&);
    // e.g. rxdir/pkg/target-config/.union/foo.a.bar+lol.cat.pch

  bool executeAction(clang::FrontendAction&, clang::FrontendInputFile&&);
  bool executeActionWithSource(clang::FrontendAction&, const string& filename, const string& s);
  bool executeActionWithFile(clang::FrontendAction&, const string& filename);

  bool setOutputFile(const string& filename);
  void clearOutputFiles();

  void setIncludePCH(const string& filename);
  void clearIncludePCH();


  // --- package interfaces ---

  bool buildPCHFileFromFile(const string& outputFilename, const string& sourceFilename);
  bool buildPCHFileFromSource(
      const string& outputFilename,
      const string& displayFilename,
      const string& source);

  string includeDirectivesForPkg(const Pkg&);
    // Returns the public interface as include directives for a package

  bool buildPkgInterface(const Pkg&, const string& PCHFilename);
    // Build the interface PCH for pkg. Returns the PCH path in `PCHFilename`

  bool buildPkgUnionInterface(
      const PkgUnionID&,
      const PkgImports&,
      const string& pkgUnionPCHFilename,
      const string& basePkgPCHFilename);
    // Like `buildPkgInterface` but implicitly builds unions as needed. The PCH path returned
    // through `PCHFilename` points to the PCH which includes all packages given in `packages`.

  bool importPkgs(const PkgImports&, string& PCHFilename, string& srcPreamble);
    // Import one or more packages. Automatically builds packages as needed.
    // If a PCH should be used, its absolute filename is written to`PCHFilename`.
    // Any preamble source code is written to `srcPreamble`. This happens if it's decided to
    // directly include a package, or if there're any package name aliasing happening.

  enum class PkgStatus {
    Error,              // An error occured and was reported
    InterfaceOutdated,  // The interface is outdated
    ImplOutdated,       // The implementation is outdated
    Outdated,           // Both the interface and the implementation are outdated
    UpToDate,           // The package is up-to date
  };
  PkgStatus checkPkg(const Pkg&);
    // Check if a package is fully built and up-to-date in relation to its source.

  enum class PkgFixes {
    All,
      // Try to fix everything, like building interface and impl.
    BuildImpl,
      // Only build the implementation if outdated.
  };
  // enum PkgFixFlags {
  //   Error          = 0, // An error occured and was reported
  //   FixedInterface = 1, // 
  //   FixedImpl      = 2, // 
  //   NothingFixed   = 3, // 
  // };
  PkgStatus checkAndFixPkg(const Pkg&, PkgFixes);
    // Run checkPkg on a package and apply fixes

  // --- package implementations ---

  bool processPkgImports(const PkgImports&, const string& filename, string& srcPreamble);

  llvm::Module* buildLLVMModuleFromSource(
      llvm::LLVMContext*,
      const string& displayFilename,
      const string& source); // 1
  llvm::Module* buildLLVMModuleFromSource(
      llvm::LLVMContext*,
      const string& displayFilename,
      const PkgImports&,
      const string& source); // 2
    // Given the current setup, compiles C++ `source` as a LLVM module, representing LLVM IR.
    // `displayFilename` is used for compiler diagnostics reporting.
    // 1) builds a pure package w/o importing or including any other packages.
    // 2) builds any dependencies and includes those dependencies when building the package.


  // llvm::Module* buildLLVMModuleFromFile(
  //     llvm::LLVMContext*,
  //     const string& filename,
  //     const PkgImports&);

private:
  struct Imp; Imp* self;
};

} // namespace
