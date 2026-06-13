//===- TinyDSP.h - Top-level interface for TinyDSP ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM TinyDSP back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYDSP_TINYDSP_H
#define LLVM_LIB_TARGET_TINYDSP_TINYDSP_H

#include "MCTargetDesc/TinyDSPMCTargetDesc.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class FunctionPass;
class TinyDSPTargetMachine;
enum class CodeGenOptLevel;

// Pass creation functions
FunctionPass *createTinyDSPISelDag(TinyDSPTargetMachine &TM,
                                   CodeGenOptLevel OptLevel);

// Custom SDNode types
namespace TinyDSPISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET,       // Return
  CALL,      // Function call
  Wrapper    // Address wrapper
};
} // namespace TinyDSPISD

} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYDSP_TINYDSP_H
