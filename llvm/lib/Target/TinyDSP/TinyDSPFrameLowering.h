//===- TinyDSPFrameLowering.h - Define frame lowering for TinyDSP -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class implements TinyDSP-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYDSP_TINYDSPFRAMELOWERING_H
#define LLVM_LIB_TARGET_TINYDSP_TINYDSPFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class TinyDSPSubtarget;

class TinyDSPFrameLowering : public TargetFrameLowering {
public:
  explicit TinyDSPFrameLowering(const TinyDSPSubtarget &STI)
      : TargetFrameLowering(StackGrowsDown,
                            /*StackAlignment=*/Align(4),
                            /*LocalAreaOffset=*/0) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasFPImpl(const MachineFunction &MF) const override;
  
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYDSP_TINYDSPFRAMELOWERING_H
