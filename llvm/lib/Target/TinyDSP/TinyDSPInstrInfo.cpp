//===- TinyDSPInstrInfo.cpp - TinyDSP Instruction Information ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the TinyDSP implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPInstrInfo.h"
#include "TinyDSP.h"
#include "TinyDSPSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "TinyDSPGenInstrInfo.inc"

TinyDSPInstrInfo::TinyDSPInstrInfo(const TinyDSPSubtarget &STI)
    : TinyDSPGenInstrInfo(STI, RI, TinyDSP::ADJCALLSTACKDOWN,
                          TinyDSP::ADJCALLSTACKUP,
                          /*CatchRetOpcode=*/0,
                          /*ReturnOpcode=*/TinyDSP::RET),
      RI() {}

void TinyDSPInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, Register DestReg,
                                   Register SrcReg, bool KillSrc,
                                   bool RenamableDest,
                                   bool RenamableSrc) const {
  // Use ADD with R0 (which is constant 0) to implement MOV
  // MOV rd, rs => ADD rd, rs, R0
  BuildMI(MBB, MI, DL, get(TinyDSP::ADD), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addReg(TinyDSP::R0);
}

void TinyDSPInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC,
  Register VReg, MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  BuildMI(MBB, MI, DL, get(TinyDSP::STORE))
      .addReg(SrcReg, getKillRegState(isKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .setMIFlag(Flags);
}

void TinyDSPInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
  int FrameIndex, const TargetRegisterClass *RC, Register VReg,
  unsigned SubReg,
    MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  BuildMI(MBB, MI, DL, get(TinyDSP::LOAD), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .setMIFlag(Flags);
}
