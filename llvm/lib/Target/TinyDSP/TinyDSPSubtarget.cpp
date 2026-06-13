//===- TinyDSPSubtarget.cpp - TinyDSP Subtarget Information --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the TinyDSP specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPSubtarget.h"
#include "TinyDSP.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "tinydsp-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "TinyDSPGenSubtargetInfo.inc"

void TinyDSPSubtarget::anchor() {}

TinyDSPSubtarget::TinyDSPSubtarget(const Triple &TT, const std::string &CPU,
                                   const std::string &FS,
                                   const TargetMachine &TM)
    : TinyDSPGenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS), InstrInfo(*this),
      TLInfo(TM, *this), FrameLowering(*this), TSInfo() {}
