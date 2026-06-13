//===- TinyDSPMCTargetDesc.cpp - TinyDSP Target Descriptions -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides TinyDSP specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCTargetDesc.h"
#include "TinyDSPInstPrinter.h"
#include "TinyDSPMCAsmInfo.h"
#include "TargetInfo/TinyDSPTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "TinyDSPGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "TinyDSPGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "TinyDSPGenRegisterInfo.inc"

static MCInstrInfo *createTinyDSPMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitTinyDSPMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createTinyDSPMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitTinyDSPMCRegisterInfo(X, TinyDSP::R0);
  return X;
}

static MCSubtargetInfo *createTinyDSPMCSubtargetInfo(const Triple &TT,
                                                     StringRef CPU,
                                                     StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";
  
  return createTinyDSPMCSubtargetInfoImpl(TT, CPUName, /*TuneCPU*/ CPUName, FS);
}

static MCInstPrinter *createTinyDSPMCInstPrinter(const Triple &T,
                                                 unsigned SyntaxVariant,
                                                 const MCAsmInfo &MAI,
                                                 const MCInstrInfo &MII,
                                                 const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new TinyDSPInstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeTinyDSPTargetMC() {
  // Register the MC asm info
  RegisterMCAsmInfo<TinyDSPMCAsmInfo> X(getTheTinyDSPTarget());

  // Register the MC instruction info
  TargetRegistry::RegisterMCInstrInfo(getTheTinyDSPTarget(),
                                      createTinyDSPMCInstrInfo);

  // Register the MC register info
  TargetRegistry::RegisterMCRegInfo(getTheTinyDSPTarget(),
                                    createTinyDSPMCRegisterInfo);

  // Register the MC subtarget info
  TargetRegistry::RegisterMCSubtargetInfo(getTheTinyDSPTarget(),
                                          createTinyDSPMCSubtargetInfo);

  // Register the MC code emitter
  TargetRegistry::RegisterMCCodeEmitter(getTheTinyDSPTarget(),
                                        createTinyDSPMCCodeEmitter);

  // Register the ASM backend
  TargetRegistry::RegisterMCAsmBackend(getTheTinyDSPTarget(),
                                       createTinyDSPAsmBackend);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(getTheTinyDSPTarget(),
                                        createTinyDSPMCInstPrinter);
}
