//===- TinyDSPMCCodeEmitter.cpp - Convert TinyDSP Code to Machine Code ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the TinyDSPMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

namespace {
class TinyDSPMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  TinyDSPMCCodeEmitter(const MCInstrInfo &mcii, MCContext &ctx)
      : MCII(mcii), Ctx(ctx) {}

  ~TinyDSPMCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned encodeMemoryOpValue(const MCInst &MI, unsigned OpNo,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;
};

} // end anonymous namespace

void TinyDSPMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                             SmallVectorImpl<char> &CB,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  uint32_t Binary = (uint32_t)getBinaryCodeForInstr(MI, Fixups, STI);
  
  // Emit instruction in little-endian
  support::endian::write<uint32_t>(CB, Binary, llvm::endianness::little);
}

unsigned
TinyDSPMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  
  // MO is an expression
  return 0;
}

unsigned
TinyDSPMCCodeEmitter::encodeMemoryOpValue(const MCInst &MI, unsigned OpNo,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  // Memory operand: base register + offset
  unsigned Encoding = 0;
  const MCOperand &BaseReg = MI.getOperand(OpNo);
  const MCOperand &Offset = MI.getOperand(OpNo + 1);
  
  Encoding |= getMachineOpValue(MI, BaseReg, Fixups, STI) << 18;
  Encoding |= getMachineOpValue(MI, Offset, Fixups, STI) & 0x3FFFF;
  
  return Encoding;
}

#include "TinyDSPGenMCCodeEmitter.inc"

MCCodeEmitter *llvm::createTinyDSPMCCodeEmitter(const MCInstrInfo &MCII,
                                                MCContext &Ctx) {
  return new TinyDSPMCCodeEmitter(MCII, Ctx);
}
