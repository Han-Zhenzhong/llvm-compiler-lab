add_llvm_component_library(LLVMTinyDSPInfo
  TinyDSPTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  TinyDSP
  )
