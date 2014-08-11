#include "compiler.hh"
#include "async.hh"
#include "net.hh"
#include "fs.hh"
#include "deps.hh"
#include <iostream>

// using namespace llvm;
using namespace clang;
using namespace rx;
using std::cerr;
using std::endl;
using std::string;

static int ExecuteLLVMModule(llvm::Module*, char*const* envp);


int main(int argc, const char **argv, char*const* envp) {
  Compiler compiler;

  // {
  //   fs::SymLink ln;
  //   auto err = fs::readlink(compiler.rxDir() + "/src/foo/bar/bar-link.cc", ln);
  //   cerr << "readlink: err=" << err << " ln.target=" << ln.target << endl;
  // }

  // rx build foo/bar
  PkgDeps pkgDeps{compiler.rxDir(), compiler.pkgDir(), "foo/bar", [](Error err) {
    if (err) {
      cerr << "pkgDeps failed: " << err << endl;
    } else {
      cerr << "pkgDeps completed successfully" << endl;
    }
  }};
  pkgDeps.resolve();

  Async::main().run();
  return 0;

  // bool ok;
  // string PCHPath;

  // ok = compiler.buildPkgInterface("std", PCHPath); if (!ok) return -1;
  // ok = compiler.buildPkgInterface("foo/bar", PCHPath); if (!ok) return -1;
  // ok = compiler.buildPkgInterface("foo/baz", PCHPath);  if (!ok) return -1;

  cerr << "Compiling user module" << endl;
  llvm::LLVMContext* llvmContext = new llvm::LLVMContext;
  auto* module = compiler.buildLLVMModuleFromSource(
    llvmContext,
    "hello.cc",
    {"std", "foo/bar", "foo/baz"},
    "#define RX_STR1(str) #str\n"
    "#define RX_STR(str) RX_STR1(str)\n"
    "\n"
    "constexpr const char* helloMessage() {\n"
    "  return (\"Hej. RX_PKG_ID=\" RX_STR(RX_PKG_ID));\n"
    "}\n"
    "int main() {\n"
    "  std::map<int,int> m{{1,2},{3,4}};\n"
    "  std::vector<int> v{1,2,3,4};\n"
    "  std::cerr << helloMessage() << std::endl;\n"
    "  return 0;\n"
    "}\n"
  );

  if (module == nullptr) {
    return -1;
  }
  auto execResult = ExecuteLLVMModule(module, envp);
  cerr << "ExecuteLLVMModule => " << execResult << endl;

  return 0;
}


// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
string GetExecutablePath(const char *Argv0) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *MainAddr = (void*) (intptr_t) GetExecutablePath;
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}


static int ExecuteLLVMModule(llvm::Module *Mod, char * const *envp) {
  llvm::InitializeNativeTarget();

  string Error;
  OwningPtr<llvm::ExecutionEngine> EE(
    llvm::ExecutionEngine::createJIT(Mod, &Error));
  if (!EE) {
    llvm::errs() << "unable to make execution engine: " << Error << "\n";
    return 255;
  }

  llvm::Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    llvm::errs() << "'main' function not found in module.\n";
    return 255;
  }

  // FIXME: Support passing arguments.
  std::vector<string> Args;
  Args.push_back(Mod->getModuleIdentifier());

  return EE->runFunctionAsMain(EntryFn, Args, envp);
}
