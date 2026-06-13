//===- TinyDSPMCTargetDesc.h - TinyDSP Target Descriptions -----*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_TINYDSP_MCTARGETDESC_TINYDSPMCTARGETDESC_H
#define LLVM_LIB_TARGET_TINYDSP_MCTARGETDESC_TINYDSPMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;

MCCodeEmitter *createTinyDSPMCCodeEmitter(const MCInstrInfo &MCII,
                                          MCContext &Ctx);

MCAsmBackend *createTinyDSPAsmBackend(const Target &T,
                                      const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createTinyDSPELFObjectWriter(uint8_t OSABI);

} // namespace llvm

// Defines symbolic names for TinyDSP registers.
#define GET_REGINFO_ENUM
#include "TinyDSPGenRegisterInfo.inc"

// Defines symbolic names for the TinyDSP instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "TinyDSPGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "TinyDSPGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_TINYDSP_MCTARGETDESC_TINYDSPMCTARGETDESC_H
