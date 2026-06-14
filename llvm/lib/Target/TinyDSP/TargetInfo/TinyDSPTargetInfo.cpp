//===- TinyDSPTargetInfo.cpp - TinyDSP Target Implementation -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/TinyDSPTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheTinyDSPTarget() {
  static Target TheTinyDSPTarget;
  return TheTinyDSPTarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeTinyDSPTargetInfo() {
  RegisterTarget<Triple::tinydsp> X(getTheTinyDSPTarget(), "tinydsp",
                                    "TinyDSP", "TinyDSP");
}
