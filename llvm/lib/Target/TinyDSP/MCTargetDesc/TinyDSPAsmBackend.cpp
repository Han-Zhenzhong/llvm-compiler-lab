//===- TinyDSPAsmBackend.cpp - TinyDSP Assembler Backend -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class TinyDSPAsmBackend : public MCAsmBackend {
public:
  TinyDSPAsmBackend(const Target &T, const MCSubtargetInfo &STI)
      : MCAsmBackend(llvm::endianness::little) {}

  void applyFixup(const MCFragment &Fragment, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createTinyDSPELFObjectWriter(0);
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if ((Count % 4) != 0)
      return false;

    for (uint64_t i = 0; i < Count; i += 4)
      OS.write("\x00\x00\x00\x00", 4);

    return true;
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createTinyDSPAsmBackend(const Target &T,
                                            const MCSubtargetInfo &STI,
                                            const MCRegisterInfo &MRI,
                                            const MCTargetOptions &Options) {
  return new TinyDSPAsmBackend(T, STI);
}
