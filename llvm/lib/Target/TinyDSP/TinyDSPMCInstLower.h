//===-- TinyDSPMCInstLower.h - Lower TinyDSP MachineInstr to MCInst ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYDSP_TINYDSPMCINSTLOWER_H
#define LLVM_LIB_TARGET_TINYDSP_TINYDSPMCINSTLOWER_H

#include "llvm/CodeGen/MachineOperand.h"

namespace llvm {

class AsmPrinter;
class MCContext;
class MCExpr;
class MCInst;
class MCOperand;
class MCSymbol;
class MachineInstr;

class TinyDSPMCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  TinyDSPMCInstLower(MCContext &C, AsmPrinter &P) : Ctx(C), Printer(P) {}

  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

private:
  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;
  MCSymbol *GetGlobalAddressSymbol(const MachineOperand &MO) const;
  MCSymbol *GetExternalSymbolSymbol(const MachineOperand &MO) const;
  MCSymbol *GetBlockAddressSymbol(const MachineOperand &MO) const;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYDSP_TINYDSPMCINSTLOWER_H
