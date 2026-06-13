add_llvm_component_library(LLVMTinyDSPDesc
  TinyDSPAsmBackend.cpp
  TinyDSPELFObjectWriter.cpp
  TinyDSPInstPrinter.cpp
  TinyDSPMCAsmInfo.cpp
  TinyDSPMCCodeEmitter.cpp
  TinyDSPMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  Support
  TinyDSPInfo

  ADD_TO_COMPONENT
  TinyDSP
  )
