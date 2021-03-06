# Requires DEPS_PREFIX to be set

execute_process(COMMAND ${DEPS_PREFIX}/bin/llvm-config --ldflags
                OUTPUT_VARIABLE LIBLLVM_LD_FLAGS)

# execute_process(COMMAND sh -c "${DEPS_PREFIX}/bin/llvm-config --libnames all | xargs -n 1 -I name echo '\"${DEPS_PREFIX}/lib/'name'\"'"
#                 OUTPUT_VARIABLE LIBLLVM_LIB_FILES)
# TODO: Figure out how run this with cmake. Argument expansion and quotation hell...
# ${DEPS_PREFIX}/bin/llvm-config --libnames all | xargs -n 1 -I name echo '"${DEPS_PREFIX}/lib/'name'"'

set(LIBLLVM_LIB_FILES
  "${DEPS_PREFIX}/lib/libLLVMInstrumentation.a"
  "${DEPS_PREFIX}/lib/libLLVMIRReader.a"
  "${DEPS_PREFIX}/lib/libLLVMAsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMDebugInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMOption.a"
  "${DEPS_PREFIX}/lib/libLLVMLTO.a"
  "${DEPS_PREFIX}/lib/libLLVMLinker.a"
  "${DEPS_PREFIX}/lib/libLLVMipo.a"
  "${DEPS_PREFIX}/lib/libLLVMVectorize.a"
  "${DEPS_PREFIX}/lib/libLLVMBitWriter.a"
  "${DEPS_PREFIX}/lib/libLLVMBitReader.a"
  "${DEPS_PREFIX}/lib/libLLVMTableGen.a"
  "${DEPS_PREFIX}/lib/libLLVMR600CodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMR600Desc.a"
  "${DEPS_PREFIX}/lib/libLLVMR600Info.a"
  "${DEPS_PREFIX}/lib/libLLVMR600AsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZDisassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZAsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMSystemZAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMHexagonCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMHexagonAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMHexagonDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMHexagonInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMNVPTXCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMNVPTXDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMNVPTXInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMNVPTXAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMCppBackendCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMCppBackendInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMMSP430CodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMMSP430Desc.a"
  "${DEPS_PREFIX}/lib/libLLVMMSP430Info.a"
  "${DEPS_PREFIX}/lib/libLLVMMSP430AsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMXCoreDisassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMXCoreCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMXCoreDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMXCoreInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMXCoreAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsDisassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsAsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMMipsAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMARMDisassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMARMCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMARMAsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMARMDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMARMInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMARMAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64Disassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64CodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64AsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64Desc.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64Info.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64AsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMAArch64Utils.a"
  "${DEPS_PREFIX}/lib/libLLVMPowerPCCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMPowerPCAsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMPowerPCDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMPowerPCInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMPowerPCAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMSparcCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMSparcDesc.a"
  "${DEPS_PREFIX}/lib/libLLVMSparcInfo.a"
  "${DEPS_PREFIX}/lib/libLLVMX86Disassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMX86AsmParser.a"
  "${DEPS_PREFIX}/lib/libLLVMX86CodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMSelectionDAG.a"
  "${DEPS_PREFIX}/lib/libLLVMAsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMX86Desc.a"
  "${DEPS_PREFIX}/lib/libLLVMX86Info.a"
  "${DEPS_PREFIX}/lib/libLLVMX86AsmPrinter.a"
  "${DEPS_PREFIX}/lib/libLLVMX86Utils.a"
  "${DEPS_PREFIX}/lib/libLLVMMCDisassembler.a"
  "${DEPS_PREFIX}/lib/libLLVMMCParser.a"
  "${DEPS_PREFIX}/lib/libLLVMInterpreter.a"
  "${DEPS_PREFIX}/lib/libLLVMMCJIT.a"
  "${DEPS_PREFIX}/lib/libLLVMJIT.a"
  "${DEPS_PREFIX}/lib/libLLVMCodeGen.a"
  "${DEPS_PREFIX}/lib/libLLVMObjCARCOpts.a"
  "${DEPS_PREFIX}/lib/libLLVMScalarOpts.a"
  "${DEPS_PREFIX}/lib/libLLVMInstCombine.a"
  "${DEPS_PREFIX}/lib/libLLVMTransformUtils.a"
  "${DEPS_PREFIX}/lib/libLLVMipa.a"
  "${DEPS_PREFIX}/lib/libLLVMAnalysis.a"
  "${DEPS_PREFIX}/lib/libLLVMRuntimeDyld.a"
  "${DEPS_PREFIX}/lib/libLLVMExecutionEngine.a"
  "${DEPS_PREFIX}/lib/libLLVMTarget.a"
  "${DEPS_PREFIX}/lib/libLLVMMC.a"
  "${DEPS_PREFIX}/lib/libLLVMObject.a"
  "${DEPS_PREFIX}/lib/libLLVMCore.a"
  "${DEPS_PREFIX}/lib/libLLVMSupport.a")


# if(VERBOSE)
# message("LLVM_LD_FLAGS:     ${LLVM_LD_FLAGS}")
# message("LIBLLVM_LIB_FILES: ${LIBLLVM_LIB_FILES}")
# endif(VERBOSE)

mark_as_advanced(LIBLLVM_LD_FLAGS LIBLLVM_LIB_FILES)
