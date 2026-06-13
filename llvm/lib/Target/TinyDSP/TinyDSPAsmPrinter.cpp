//===-- TinyDSPAsmPrinter.cpp - TinyDSP LLVM assembly writer -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCInstLower.h"
#include "TinyDSPTargetMachine.h"
#include "TargetInfo/TinyDSPTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class TinyDSPAsmPrinter : public AsmPrinter {
public:
  explicit TinyDSPAsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "TinyDSP Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;

  static char ID;
};

} // namespace

void TinyDSPAsmPrinter::emitInstruction(const MachineInstr *MI) {
  TinyDSP_MC::verifyInstructionPredicates(MI->getOpcode(),
                                          getSubtargetInfo().getFeatureBits());

  TinyDSPMCInstLower Lowering(OutContext, *this);

  MachineBasicBlock::const_instr_iterator I = MI->getIterator();
  MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();

  do {
    MCInst TmpInst;
    Lowering.Lower(&*I, TmpInst);
    OutStreamer->emitInstruction(TmpInst, getSubtargetInfo());
  } while ((++I != E) && I->isInsideBundle());
}

char TinyDSPAsmPrinter::ID = 0;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeTinyDSPAsmPrinter() {
  RegisterAsmPrinter<TinyDSPAsmPrinter> X(getTheTinyDSPTarget());
}
