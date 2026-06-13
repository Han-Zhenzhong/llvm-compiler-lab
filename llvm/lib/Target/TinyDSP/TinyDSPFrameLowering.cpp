//===- TinyDSPFrameLowering.cpp - TinyDSP Frame Information --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the TinyDSP implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPFrameLowering.h"
#include "TinyDSPInstrInfo.h"
#include "TinyDSPSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

bool TinyDSPFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken();
}

void TinyDSPFrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TinyDSPInstrInfo &TII =
      *static_cast<const TinyDSPInstrInfo *>(MF.getSubtarget().getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  // Get the number of bytes to allocate from the FrameInfo
  uint64_t StackSize = MFI.getStackSize();

  if (StackSize == 0)
    return;

  // Adjust stack pointer: SP = SP - StackSize
  // ADDI R7, R7, -StackSize
  BuildMI(MBB, MBBI, DL, TII.get(TinyDSP::ADDI), TinyDSP::R7)
      .addReg(TinyDSP::R7)
      .addImm(-StackSize);

  // Save frame pointer if needed
  if (hasFP(MF)) {
    // STORE R6, [R7 + 0]
    BuildMI(MBB, MBBI, DL, TII.get(TinyDSP::STORE))
        .addReg(TinyDSP::R6)
        .addReg(TinyDSP::R7)
        .addImm(0);

    // MOV R6, R7 (FP = SP)
    BuildMI(MBB, MBBI, DL, TII.get(TinyDSP::ADD), TinyDSP::R6)
        .addReg(TinyDSP::R7)
        .addReg(TinyDSP::R0);
  }
}

void TinyDSPFrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TinyDSPInstrInfo &TII =
      *static_cast<const TinyDSPInstrInfo *>(MF.getSubtarget().getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  DebugLoc DL = MBBI->getDebugLoc();

  uint64_t StackSize = MFI.getStackSize();

  if (StackSize == 0)
    return;

  // Restore frame pointer if needed
  if (hasFP(MF)) {
    // LOAD R6, [R7 + 0]
    BuildMI(MBB, MBBI, DL, TII.get(TinyDSP::LOAD), TinyDSP::R6)
        .addReg(TinyDSP::R7)
        .addImm(0);
  }

  // Adjust stack pointer: SP = SP + StackSize
  // ADDI R7, R7, StackSize
  BuildMI(MBB, MBBI, DL, TII.get(TinyDSP::ADDI), TinyDSP::R7)
      .addReg(TinyDSP::R7)
      .addImm(StackSize);
}

MachineBasicBlock::iterator TinyDSPFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  // Simply remove the pseudo instruction
  return MBB.erase(I);
}
