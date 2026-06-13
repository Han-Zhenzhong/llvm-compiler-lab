//===- TinyDSPTargetMachine.cpp - Define TargetMachine for TinyDSP -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about TinyDSP target spec.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPTargetMachine.h"
#include "TinyDSP.h"
#include "TargetInfo/TinyDSPTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include <optional>

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeTinyDSPTarget() {
  // Register the target
  RegisterTargetMachine<TinyDSPTargetMachine> X(getTheTinyDSPTarget());
}

static std::string computeDataLayout(const Triple &TT) {
  // TinyDSP is little endian, 32-bit architecture
  return "e"        // Little endian
         "-m:e"     // ELF name mangling
         "-p:32:32" // 32-bit pointers, 32-bit aligned
         "-i64:64"  // 64-bit integers, 64-bit aligned
         "-n32"     // 32-bit native integer width
         "-S32";    // 32-bit stack alignment
}

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

TinyDSPTargetMachine::TinyDSPTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, computeDataLayout(TT), TT, CPU, FS, Options,
                               getEffectiveRelocModel(RM),
                               getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

namespace {
/// TinyDSP Code Generator Pass Configuration Options.
class TinyDSPPassConfig : public TargetPassConfig {
public:
  TinyDSPPassConfig(TinyDSPTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  TinyDSPTargetMachine &getTinyDSPTargetMachine() const {
    return getTM<TinyDSPTargetMachine>();
  }

  bool addInstSelector() override;
};
} // namespace

TargetPassConfig *
TinyDSPTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new TinyDSPPassConfig(*this, PM);
}

bool TinyDSPPassConfig::addInstSelector() {
  addPass(createTinyDSPISelDag(getTinyDSPTargetMachine(), getOptLevel()));
  return false;
}
