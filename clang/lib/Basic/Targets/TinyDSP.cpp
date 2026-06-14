//===--- TinyDSP.cpp - Implement TinyDSP target feature support ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements TinyDSP TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "TinyDSP.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

const char *const TinyDSPTargetInfo::GCCRegNames[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"};

ArrayRef<const char *> TinyDSPTargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

void TinyDSPTargetInfo::getTargetDefines(const LangOptions &Opts,
                                         MacroBuilder &Builder) const {
  Builder.defineMacro("__tinydsp__");
  Builder.defineMacro("__TINYDSP__");
}
