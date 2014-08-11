// Note: we use PCH prefix.hh which includes most of llvm and clang interfaces
#include "compiler.hh"
#include "join.hh"
#include "time.hh"
#include "deps.hh"

using namespace llvm;
using namespace clang;
// using namespace clang::driver;
using std::cerr;
using std::endl;

namespace rx {

struct Compiler::Imp {
  CompilerInstance   compiler;
  string             targetTriple;

  string             moduleHash;  
    // A module hash string that is suitable for uniquely identifying the conditions under which
    // the module was built. Takes things like language and code generation options into account.

  string             rxDir;
  string             pkgDir;
    // Where compiled packages are stored for the current target configuration

  Imp()
    : targetTriple{llvm::Triple::normalize(llvm::sys::getProcessTriple())}
    {}
};


Compiler::~Compiler() {
  // TODO
  delete self;
  self = nullptr;
}


Compiler::Compiler() : self{new Imp} {
  auto& compiler = self->compiler;

  // Diagnostics
  compiler.createDiagnostics();
  assert(compiler.hasDiagnostics());
  auto& diagEngine = compiler.getDiagnostics();
  auto& diagOpts = diagEngine.getDiagnosticOptions();
  diagOpts.ShowColors = true;
  diagOpts.ErrorLimit = 15;
  diagOpts.TabStop = 2;
  diagOpts.Warnings.emplace_back("all");
    // Note: This is NOT the same as diagEngine.setEnableAllWarnings(true), which is equivalent
    // to "everything"
  auto* diagClient = new TextDiagnosticPrinter(llvm::errs(), &diagEngine.getDiagnosticOptions());
  diagEngine.setClient(diagClient, /*ShouldOwnClient=*/true);
  diagEngine.setSuppressSystemWarnings(true);

  // Target
  auto& targetOpts = compiler.getTargetOpts();
  targetOpts.Triple = self->targetTriple;

  // Language
  auto& langOpts = compiler.getLangOpts();
  langOpts.C99 = true;
  langOpts.C11 = true;
  langOpts.CPlusPlus = true;
  langOpts.CPlusPlus11 = true;
  langOpts.CPlusPlus1y = true; // c++14
  langOpts.WChar = true;
  langOpts.LineComment = true; // allow "//"
  langOpts.ImplicitInt = false;
  langOpts.Bool = true; // bool, true, and false keywords
  langOpts.RTTI = false; // no RTTI
  langOpts.PICLevel = 0;
  langOpts.PIELevel = 0;
  langOpts.FastMath = true;
  // langOpts.POSIXThreads = true;
  assert(compiler.getLangOpts().CPlusPlus1y == true);

  // Code generation
  auto& codeGenOpts = compiler.getCodeGenOpts();
  codeGenOpts.Autolink = false;
  // codeGenOpts.OptimizationLevel = 4; // [0-4]

  // Frontend
  auto& frontendOpts = compiler.getFrontendOpts();
  frontendOpts.RelocatablePCH = true; // Generate relocatable PCHs

  // Header search options (See CompilerInvocation::ParseHeaderSearchArgs source for example)
  auto& headerOpts = compiler.getHeaderSearchOpts();
  headerOpts.UseStandardSystemIncludes = true;
  headerOpts.UseStandardCXXIncludes = false;
  headerOpts.UseBuiltinIncludes = true;
  headerOpts.UseLibcxx = true;
  headerOpts.ResourceDir =
    "/Users/rasmus/src2/rx/minilang/clang/product/lib/clang/3.4"; // for built-ins
  headerOpts.AddPath(
    "/Users/rasmus/src2/rx/minilang/clang/product/include/c++/v1",
    frontend::IncludeDirGroup::CXXSystem,
    /*IsFramework=*/false,
    /*IgnoreSysRoot=*/true
  );
  // headerOpts.Verbose = true; // Prints search locations

  // Dependency output
  auto& depsOpts = compiler.getDependencyOutputOpts();
  depsOpts.IncludeSystemHeaders = true;
  // depsOpts.ShowHeaderIncludes = true; // Prints include hierarchy
  // depsOpts.DOTOutputFile = "deps.dot"; // Write Graphviz header dependencies to file
  // depsOpts.OutputFile = "deps.d"; // Write ".d" header dependencies to file

  // Target
  compiler.setTarget(
    TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), &compiler.getTargetOpts())
  );
  compiler.getTarget().setForcedLangOptions(compiler.getLangOpts());
  // CompilerInvocation::CreateFromArgs(Invocation, argv, argv + argc, Clang.getDiagnostics());
  // cerr << "getTargetDescription: " << compiler.getTarget().getTargetDescription() << endl;

  // File system
  compiler.createFileManager();
  compiler.createSourceManager(compiler.getFileManager());

  // Preprocessor
  // Create our own instead of having the compiler create one during ExecuteAction.
  #if 0
  HeaderSearch* headerSearch = new HeaderSearch{
    &compiler.getHeaderSearchOpts(),
    compiler.getSourceManager(),
    compiler.getDiagnostics(),
    compiler.getLangOpts(),
    &compiler.getTarget()
  };
  Preprocessor* PP = new Preprocessor{
    &compiler.getPreprocessorOpts(),
    compiler.getDiagnostics(),
    compiler.getLangOpts(),
    &compiler.getTarget(),
    compiler.getSourceManager(),
    *headerSearch,
    compiler,
    /*IILookup=*/0,
    OwnsHeaderSearch=true
  };
  compiler.setPreprocessor(PP); // takes ownership
  compiler.createASTContext(); // needed if we create our own preprocessor
  assert(PP->getLangOpts().CPlusPlus1y == true);
  #endif

  // rxdir
  char cwdbuf[PATH_MAX];
  self->rxDir = string{::getcwd(cwdbuf, sizeof(cwdbuf))} + "/rxdir";

  // Generate module hash by creating a temporary CompilerInvocation. We do this because
  // CompilerInvocation impl contains the actual hash generation code.
  CompilerInvocation invocation;
  *invocation.getLangOpts() = compiler.getLangOpts();
  invocation.getTargetOpts() = compiler.getTargetOpts();
  invocation.getDiagnosticOpts() = compiler.getDiagnosticOpts();
  invocation.getHeaderSearchOpts() = compiler.getHeaderSearchOpts();
  invocation.getPreprocessorOpts() = compiler.getPreprocessorOpts();
  invocation.getCodeGenOpts() = compiler.getCodeGenOpts();
  invocation.getDependencyOutputOpts() = compiler.getDependencyOutputOpts();
  invocation.getFileSystemOpts() = compiler.getFileSystemOpts();
  invocation.getFrontendOpts() = compiler.getFrontendOpts();
  invocation.getPreprocessorOutputOpts() = compiler.getPreprocessorOutputOpts();

  self->moduleHash = invocation.getModuleHash();
  string pkgID = self->targetTriple + "-" + self->moduleHash;
  self->pkgDir = self->rxDir + "/pkg/" + pkgID;
    // FIXME TODO actual path
  cerr << "pkgDir: " << self->pkgDir << endl;

  // Add to system header search paths
  // headerOpts.AddPath(
  //   self->rxDir + "/src",
  //   frontend::IncludeDirGroup::CXXSystem,
  //   /*IsFramework=*/false,
  //   /*IgnoreSysRoot=*/true
  // );

  // Add to PP definitions
  auto& prepOpts = compiler.getPreprocessorOpts();
  prepOpts.addMacroDef(string{"RX_PKG_ID="} + pkgID);
  prepOpts.DisablePCHValidation = true; // because we built it ourselves
}

const string& Compiler::rxDir() const { return self->rxDir; }
const string& Compiler::pkgDir() const { return self->pkgDir; }

string Compiler::srcPathForPkg(const Pkg& pkg) {
  return self->rxDir + "/src/" + pkg.name();
}

string Compiler::binPathForPkg(const Pkg& pkg) {
  return self->pkgDir + "/" + pkg.name();
}

string Compiler::PCHPathForPkg(const Pkg& pkg) {
  return binPathForPkg(pkg) + ".pch";
}

string Compiler::PCHPathForPkgUnionID(const PkgUnionID& pkgUnionID) {
  return self->pkgDir + "/.union/" + pkgUnionID.toString() + ".pch";
}


bool Compiler::executeAction(FrontendAction& action, FrontendInputFile&& inputFile) {
  #if 1
  self->compiler.getFrontendOpts().Inputs.emplace_back(inputFile);
    // Need to set this for chained PCH to work, as it accesses
    // self->compiler.getFrontendOpts().Inputs[0].getKind() internally.
  auto success = self->compiler.ExecuteAction(action);
  self->compiler.getFrontendOpts().Inputs.clear();
  #else
  action.setCompilerInstance(&self->compiler);
  FrontendInputFile inputFile{displayFilename, InputKind::IK_CXX};
  auto r = action.BeginSourceFile(self->compiler, inputFile);
  assert(r == true);
  auto success = action.Execute();
  action.EndSourceFile();
  #endif

  clearIncludePCH();
  clearOutputFiles();
  return success;
}


bool Compiler::executeActionWithSource(
    FrontendAction& action,
    const string& filename,
    const string& source)
{
  auto& prepOpts = self->compiler.getPreprocessorOpts();
  prepOpts.addRemappedFile(filename, llvm::MemoryBuffer::getMemBuffer(source));
  return executeAction(action, {filename, InputKind::IK_CXX});
}


bool Compiler::executeActionWithFile(FrontendAction& action, const string& filename) {
  return executeAction(action, {filename, InputKind::IK_CXX});
}


void Compiler::clearOutputFiles() {
  self->compiler.clearOutputFiles(false);
  self->compiler.getFrontendOpts().OutputFile.clear();
  self->compiler.getPreprocessorOpts().clearRemappedFiles();
}


bool Compiler::setOutputFile(const string& filename) {
  self->compiler.getFrontendOpts().OutputFile = filename;

  // Track the output file and create any intermediate directories
  string Error, OutputPathName, TempPathName;
  llvm::raw_fd_ostream* OS = self->compiler.createOutputFile(
    filename,
    Error,
    /*Binary=*/true,
    /*RemoveFileOnSignal=*/true, // not thread-safe as it relies on signals (doh!)
    /*InFile=*/"",
    /*Extension=*/"",
    /*UseTemporary=*/true,
    /*CreateMissingDirectories=*/true,
    &OutputPathName,
    &TempPathName
  );
  if (!OS) {
    self->compiler.getDiagnostics().Report(diag::err_fe_unable_to_open_output) << filename << Error;
    clearOutputFiles();
    return false;
  }
  self->compiler.addOutputFile({ (OutputPathName != "-") ? OutputPathName : "", TempPathName, OS });

  return true;
}


void Compiler::setIncludePCH(const string& filename) {
  assert(self->compiler.getPreprocessorOpts().ImplicitPCHInclude.empty());
  self->compiler.getPreprocessorOpts().ImplicitPCHInclude = filename;
}

void Compiler::clearIncludePCH() {
  self->compiler.getPreprocessorOpts().ImplicitPCHInclude.clear();
}


bool Compiler::buildPCHFileFromSource(
    const string& outputFilename,
    const string& displayFilename,
    const string& source)
{
  setOutputFile(outputFilename);
  GeneratePCHAction action;
  return executeActionWithSource(action, displayFilename, source);
}


bool Compiler::buildPCHFileFromFile(const string& outputFilename, const string& sourceFilename) {
  setOutputFile(outputFilename);
  GeneratePCHAction action;
  return executeActionWithFile(action, sourceFilename);
}


// Contains mappings from standard packages to include directives as
// std::map<string pkgName, string includeDirectives>
#include "stdpkgmap.inc"

static const string* FindStdPkgSource(const Pkg& pkg) {
  auto I = kStdPkgMap.find(pkg.name());
  return (I != kStdPkgMap.end()) ? &I->second : nullptr;
}


string Compiler::includeDirectivesForPkg(const Pkg& pkg) {
  // Standard packages
  auto* s = FindStdPkgSource(pkg);
  if (s != nullptr) return *s;

  // User packages
  // TODO: Locate in file system.
  // E.g. foo/a/bar might 
  auto srcPath = srcPathForPkg(pkg) + ".h"; // e.g. "rxdir/src/foo/a/bar.h"
  if (srcPath.empty()) {
    cerr << "Unknown package '" << pkg << "'" << endl;
    return {};
  }
  return "#include \"" + srcPath + "\"";
}


static bool FileExists(const std::string& filename, Time* mtimeOut=nullptr) {
  struct stat st;
  if (::stat(filename.c_str(), &st) == 0 &&
     (((st.st_mode & S_IFMT) == S_IFREG) || ((st.st_mode & S_IFMT) == S_IFLNK)) )
  {
    if (mtimeOut) *mtimeOut = st.st_mtimespec;
    return true;
  }
  return false;
}


Compiler::PkgStatus Compiler::checkPkg(const Pkg& pkg/*, const Time& parentMTime*/) {
  auto pkgPCH = PCHPathForPkg(pkg);
  cerr << "Checking package " << pkg << "  pch=" << pkgPCH << endl;
  Time mtime;
  if (FileExists(pkgPCH, &mtime)) {
    cerr << "up-to-date" << endl;
    // TODO: check source for changes to interface and implementation
    return PkgStatus::UpToDate;
  } else {
    if (FindStdPkgSource(pkg) != nullptr) {
      // Built-in packages have no source
      return PkgStatus::Outdated;
    } else {
      // TODO: check if source exists, or:
      // cerr << "Unknown package '" << pkg << "'" << endl;
      // return PkgStatus::Error;
      return PkgStatus::InterfaceOutdated;
    }
  }
}


Compiler::PkgStatus Compiler::checkAndFixPkg(const Pkg& pkg, PkgFixes fixes) {
  auto r = checkPkg(pkg);
  switch (r) {
    case PkgStatus::ImplOutdated:
    case PkgStatus::InterfaceOutdated:
    case PkgStatus::Outdated: {

      if (fixes == PkgFixes::All) {
        // Rebuild interface
        auto pkgPCH = PCHPathForPkg(pkg);
        if (!buildPkgInterface(pkg, pkgPCH)) {
          return PkgStatus::Error;
        }
      }

      // TODO: rebuild impl

      break;
    }

    case PkgStatus::Error:
    case PkgStatus::UpToDate: break;
  }
  return std::move(r);
}


bool Compiler::importPkgs(
    const PkgImports& packages,
    string&           PCHFilenameOut,
    string&           srcPreambleOut)
{
  assert(!packages.empty());
  // One day we will perform epic dependency resolution here...  :-o
  // For now let's just do it the blunt and slow way.

  // First of we need a single package interface (which might or might not be used in as the base
  // of a union.) It's important that this PCH exists -- one might think that we should just check
  // for a union PCH if we need a union PCH, but since union PCHs actually *reference* its base PCH,
  // we require the base PCH to exist.
  auto basePkg = *packages.cbegin();
  auto basePkgStatus = checkAndFixPkg(basePkg, PkgFixes::All);
  if (basePkgStatus == PkgStatus::Error) return false;

  auto basePkgPCH = PCHPathForPkg(basePkg);

  if (packages.size() > 1) {
    // We need a package union interface.

    // Check all packages in addition to the base package
    std::vector<std::pair<const Pkg*,PkgStatus>> outdatedPkgs;
    std::vector<const Pkg*>                      outdatedPkgInterfaces;
    std::vector<const Pkg*>                      outdatedPkgImpls;

    auto pkgI = packages.cbegin();
    auto pkgE = packages.cend();
    while (++pkgI != pkgE) {
      // Fix only missing implementations
      auto r = checkAndFixPkg(*pkgI, PkgFixes::BuildImpl);
      switch (r) {
        case PkgStatus::Error: return false;
        case PkgStatus::Outdated: {
          outdatedPkgs.emplace_back(&*pkgI, r);
          outdatedPkgInterfaces.emplace_back(&*pkgI);
          outdatedPkgImpls.emplace_back(&*pkgI);
          break;
        }
        case PkgStatus::InterfaceOutdated: {
          outdatedPkgs.emplace_back(&*pkgI, r);
          outdatedPkgInterfaces.emplace_back(&*pkgI);
          break;
        }
        case PkgStatus::ImplOutdated: {
          outdatedPkgs.emplace_back(&*pkgI, r);
          outdatedPkgImpls.emplace_back(&*pkgI);
          break;
        }
        default: break;
      }
    }

    // debug print outdated interfaces
    for (auto& p : outdatedPkgs) {
      cerr << "[pkg union check] outdated package " << *p.first << ": "
           << ((p.second == PkgStatus::InterfaceOutdated) ? "interface" :
               (p.second == PkgStatus::ImplOutdated)      ? "implementation" :
               (p.second == PkgStatus::Outdated)          ? "interface & implementation" :
               "UNEXPECTED")
           << endl;
    }

    PkgUnionID pkgUnionID(packages);
    auto pkgUnionPCH = PCHPathForPkgUnionID(pkgUnionID);

    if (basePkgStatus == PkgStatus::UpToDate) cerr << "Checking for " << pkgUnionPCH << endl; // DBG

    if (basePkgStatus != PkgStatus::UpToDate ||
        !outdatedPkgInterfaces.empty() ||
        !FileExists(pkgUnionPCH))
    {
      // The base of the union has been modified, or the union interface doesn't exist.
      // In the future we should try to be clever and #include some packages.
      if (!buildPkgUnionInterface(pkgUnionID, packages, pkgUnionPCH, basePkgPCH)) {
        return false;
      }
    }

    // Union package
    PCHFilenameOut.assign(pkgUnionPCH);

  } else {
    // Single package
    PCHFilenameOut.assign(basePkgPCH);
  }

  return true;
}


bool Compiler::buildPkgInterface(const Pkg& pkg, const string& pkgPCHFilename) {
  // TODO: Inspect package interface source and compare it with any existing PCH, and if dates
  // are match, then don't recompile.
  auto source = includeDirectivesForPkg(pkg);
  if (source.empty()) return false; // error
  cerr << "Building package " << pkg.name() << endl;
  return buildPCHFileFromSource(pkgPCHFilename, "pkg:"+pkg.name()+".pch", source);
}


bool Compiler::buildPkgUnionInterface(
    const PkgUnionID& pkgUnionID,
    const PkgImports& packages,
    const string&     pkgUnionPCHFilename,
    const string&     basePkgPCHFilename)
{
  assert(packages.size() > 1);
  auto pkgI = packages.cbegin();
  auto pkgE = packages.cend();
  cerr << "Building package union (" << join(pkgI, pkgE, ", ") << ")" << endl;

  // Set the base package interface as the PCH
  assert(FileExists(basePkgPCHFilename)); // or we called this function w/o making sure it's built
  setIncludePCH(basePkgPCHFilename);

  // Combine sources for all but the parent package through includes
  string unionSource;
  ++pkgI; // skip first package, which we will PCH we will include
  for (;pkgI != pkgE; ++pkgI) {
    auto incd = includeDirectivesForPkg(*pkgI);
    if (incd.empty()) return false; // error
    unionSource += incd + "\n";
  }

  return buildPCHFileFromSource(
    pkgUnionPCHFilename,
    "union:"+pkgUnionID.toString()+".pch",
    unionSource
  );

  // Approach 1: Creates and uses intermediate unions
  //
  // pkgUnionIsInvalid(iostream+map+vector)
  //   pkgUnionIsInvalid(iostream+map)
  //     pkgUnionIsInvalid(iostream)
  //       generatePkgPCH(iostream)
  //     generatePkgUnionPCH(map, iostream)      // -include-pch=iostream  map.h
  //   generatePkgUnionPCH(vector, iostream+map) // -include-pch=iostream+map  vector.h
  //
  //
  // Approach 2: Canonical union
  //
  // pkgUnionIsInvalid(iostream+map+vector)
  //   pkgUnionIsInvalid(iostream)
  //     generatePkgPCH(iostream)
  //   generatePkgUnionPCH(map, vector, iostream) // -include-pch=iostream  map.h vector.h
  //
  // We're going with approach 2 for now.
}


llvm::Module* Compiler::buildLLVMModuleFromSource(
    llvm::LLVMContext* llvmContext,
    const string& displayFilename,
    const string& source)
{
  // Execute and check results
  // SyntaxOnlyAction action;
  EmitLLVMOnlyAction action{llvmContext};
  auto success = executeActionWithSource(action, displayFilename, source);
  if (!success) {
    auto& diag = self->compiler.getDiagnostics();
    cerr << "diag.hasErrorOccurred() = " << (diag.hasErrorOccurred() ? "true" : "false") << endl;
    cerr << "diag.getNumWarnings()   = " << diag.getNumWarnings() << endl;
    return nullptr;
  }

  // Retrieve and returns the LLVM module
  llvm::Module* module = action.takeModule(); // Take over ownership from action
  action.takeLLVMContext();
  if (module) {
    module->setTargetTriple(self->compiler.getTargetOpts().Triple);
  }

  return module;
}


bool Compiler::processPkgImports(const PkgImports& pkgs, const string& filename, string& srcPreamble) {
  // Returns source preamble
  if (pkgs.empty()) return true;

  srcPreamble.clear();
  string PCHFilename;

  if (!importPkgs(pkgs, PCHFilename, srcPreamble)) return false;

  // Prepend any source preamble
  if (!srcPreamble.empty()) {
    srcPreamble =
      std::string{"#line 1 \"rx-preamble\"\n"} +
      srcPreamble +
      "#line 1 \"" + filename + "\"\n";
  }
  // TODO: include any package *implementations* through compiling what's needed and linking

  setIncludePCH(PCHFilename); // OK if empty
  return true;
}


llvm::Module* Compiler::buildLLVMModuleFromSource(
    llvm::LLVMContext*  llvmContext,
    const string&       filename,
    const PkgImports&   pkgImports,
    const string&       source)
{
  // TODO: derive pkg from filename
  // Pkg pkg{"foo/hello"};
  // PkgDeps pkgDeps{self->rxDir, self->pkgDir, pkg, pkgImports};
  // pkgDeps.resolve();
  // return nullptr; // XXX DEBUG

  string srcPreamble;
  if (!processPkgImports(pkgImports, filename, srcPreamble)) return nullptr;

  // Prepend any source preamble
  string modifiedSource;
  if (!srcPreamble.empty()) {
    modifiedSource = srcPreamble + source;
  }

  return buildLLVMModuleFromSource(
    llvmContext,
    filename,
    modifiedSource.empty() ? source : modifiedSource
  );
}


} // namespace
