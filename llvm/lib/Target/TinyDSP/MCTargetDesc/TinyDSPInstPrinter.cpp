//===- TinyDSPInstPrinter.cpp - Convert TinyDSP MCInst to asm ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints a TinyDSP MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPInstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "TinyDSPGenAsmWriter.inc"

void TinyDSPInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void TinyDSPInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    O << getRegisterName(Op.getReg());
  } else if (Op.isImm()) {
    O << Op.getImm();
  } else {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    MAI.printExpr(O, *Op.getExpr());
  }
}

void TinyDSPInstPrinter::printMemOperand(const MCInst *MI, unsigned OpNo,
                                         raw_ostream &O) {
  O << "[";
  printOperand(MI, OpNo, O);
  
  const MCOperand &OffsetOp = MI->getOperand(OpNo + 1);
  if (OffsetOp.isImm() && OffsetOp.getImm() != 0) {
    O << " + ";
    O << OffsetOp.getImm();
  }
  O << "]";
}
