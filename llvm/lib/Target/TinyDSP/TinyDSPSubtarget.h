//===- TinyDSPSubtarget.h - Define Subtarget for TinyDSP -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the TinyDSP specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYDSP_TINYDSPSUBTARGET_H
#define LLVM_LIB_TARGET_TINYDSP_TINYDSPSUBTARGET_H

#include "TinyDSPFrameLowering.h"
#include "TinyDSPISelLowering.h"
#include "TinyDSPInstrInfo.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "TinyDSPGenSubtargetInfo.inc"

namespace llvm {
class StringRef;

class TinyDSPSubtarget : public TinyDSPGenSubtargetInfo {
  virtual void anchor();
  TinyDSPInstrInfo InstrInfo;
  TinyDSPTargetLowering TLInfo;
  TinyDSPFrameLowering FrameLowering;
  SelectionDAGTargetInfo TSInfo;

public:
  TinyDSPSubtarget(const Triple &TT, const std::string &CPU,
                   const std::string &FS, const TargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const TinyDSPInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const TinyDSPFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const TinyDSPTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  const TargetRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYDSP_TINYDSPSUBTARGET_H
