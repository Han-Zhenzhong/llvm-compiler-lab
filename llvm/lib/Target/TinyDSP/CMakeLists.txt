add_llvm_component_group(TinyDSP)

set(LLVM_TARGET_DEFINITIONS TinyDSP.td)

tablegen(LLVM TinyDSPGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM TinyDSPGenCallingConv.inc -gen-callingconv)
tablegen(LLVM TinyDSPGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM TinyDSPGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM TinyDSPGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM TinyDSPGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM TinyDSPGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(TinyDSPCommonTableGen)

add_llvm_target(TinyDSPCodeGen
  TinyDSPFrameLowering.cpp
  TinyDSPInstrInfo.cpp
  TinyDSPISelDAGToDAG.cpp
  TinyDSPISelLowering.cpp
  TinyDSPRegisterInfo.cpp
  TinyDSPSubtarget.cpp
  TinyDSPTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TinyDSPDesc
  TinyDSPInfo

  ADD_TO_COMPONENT
  TinyDSP
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
