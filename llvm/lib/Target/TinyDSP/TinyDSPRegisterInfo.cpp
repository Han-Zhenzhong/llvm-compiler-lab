//===- TinyDSPRegisterInfo.cpp - TinyDSP Register Information ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the TinyDSP implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPRegisterInfo.h"
#include "TinyDSP.h"
#include "TinyDSPSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "TinyDSPGenRegisterInfo.inc"

TinyDSPRegisterInfo::TinyDSPRegisterInfo() : TinyDSPGenRegisterInfo(TinyDSP::R0) {}

const MCPhysReg *
TinyDSPRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_SaveList;
}

BitVector TinyDSPRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  
  // Reserve R7 as stack pointer
  Reserved.set(TinyDSP::R7);
  
  // Reserve R6 as frame pointer
  Reserved.set(TinyDSP::R6);
  
  return Reserved;
}

bool TinyDSPRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MF.getFrameInfo().getObjectOffset(FrameIndex) +
               MI.getOperand(FIOperandNum + 1).getImm() +
               TFI->getFrameIndexReferenceFromSP(MF, FrameIndex).getFixed();

  // Replace frame index with base register (R7 = SP) + offset
  MI.getOperand(FIOperandNum).ChangeToRegister(TinyDSP::R7, false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);

  return false;
}

Register TinyDSPRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return TinyDSP::R6;
}
