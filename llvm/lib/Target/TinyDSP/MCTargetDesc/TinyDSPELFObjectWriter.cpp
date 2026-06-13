//===- TinyDSPELFObjectWriter.cpp - TinyDSP ELF Writer -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class TinyDSPELFObjectWriter : public MCELFObjectTargetWriter {
public:
  TinyDSPELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit*/ false, OSABI, ELF::EM_NONE,
                                /*HasRelocationAddend*/ false) {}

  ~TinyDSPELFObjectWriter() override = default;

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    // No relocations yet
    return ELF::R_386_NONE;
  }
};
} // end anonymous namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createTinyDSPELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<TinyDSPELFObjectWriter>(OSABI);
}
