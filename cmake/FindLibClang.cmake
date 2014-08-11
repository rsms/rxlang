# Requires DEPS_PREFIX to be set

set(LIBCLANG_STATIC_LIBS
  "${DEPS_PREFIX}/lib/libclang.a"
  "${DEPS_PREFIX}/lib/libclangARCMigrate.a"
  "${DEPS_PREFIX}/lib/libclangAST.a"
  "${DEPS_PREFIX}/lib/libclangASTMatchers.a"
  "${DEPS_PREFIX}/lib/libclangAnalysis.a"
  "${DEPS_PREFIX}/lib/libclangApplyReplacements.a"
  "${DEPS_PREFIX}/lib/libclangBasic.a"
  "${DEPS_PREFIX}/lib/libclangCodeGen.a"
  "${DEPS_PREFIX}/lib/libclangDriver.a"
  "${DEPS_PREFIX}/lib/libclangDynamicASTMatchers.a"
  "${DEPS_PREFIX}/lib/libclangEdit.a"
  "${DEPS_PREFIX}/lib/libclangFormat.a"
  "${DEPS_PREFIX}/lib/libclangFrontend.a"
  "${DEPS_PREFIX}/lib/libclangFrontendTool.a"
  "${DEPS_PREFIX}/lib/libclangIndex.a"
  "${DEPS_PREFIX}/lib/libclangLex.a"
  "${DEPS_PREFIX}/lib/libclangParse.a"
  "${DEPS_PREFIX}/lib/libclangQuery.a"
  "${DEPS_PREFIX}/lib/libclangRewriteCore.a"
  "${DEPS_PREFIX}/lib/libclangRewriteFrontend.a"
  "${DEPS_PREFIX}/lib/libclangSema.a"
  "${DEPS_PREFIX}/lib/libclangSerialization.a"
  "${DEPS_PREFIX}/lib/libclangStaticAnalyzerCheckers.a"
  "${DEPS_PREFIX}/lib/libclangStaticAnalyzerCore.a"
  "${DEPS_PREFIX}/lib/libclangStaticAnalyzerFrontend.a"
  "${DEPS_PREFIX}/lib/libclangTidy.a"
  "${DEPS_PREFIX}/lib/libclangTidyGoogleModule.a"
  "${DEPS_PREFIX}/lib/libclangTidyLLVMModule.a"
  "${DEPS_PREFIX}/lib/libclangTooling.a")

mark_as_advanced(LIBCLANG_STATIC_LIBS)
