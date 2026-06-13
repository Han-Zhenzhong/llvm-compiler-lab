//===- TinyDSPISelLowering.cpp - TinyDSP DAG Lowering Implementation -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the TinyDSPTargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPISelLowering.h"
#include "TinyDSP.h"
#include "TinyDSPSubtarget.h"
#include "TinyDSPTargetMachine.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "tinydsp-lower"

TinyDSPTargetLowering::TinyDSPTargetLowering(const TargetMachine &TM,
                                             const TinyDSPSubtarget &STI)
  : TargetLowering(TM, STI), Subtarget(STI) {

  // Set up the register classes
  addRegisterClass(MVT::i32, &TinyDSP::GPRRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(TinyDSP::R7);

  // Basic operations
  setOperationAction(ISD::BR_CC, MVT::i32, Expand);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);
  setOperationAction(ISD::SELECT, MVT::i32, Expand);
  setOperationAction(ISD::SETCC, MVT::i32, Expand);

  // Division and remainder
  setOperationAction(ISD::SDIV, MVT::i32, Expand);
  setOperationAction(ISD::UDIV, MVT::i32, Expand);
  setOperationAction(ISD::SREM, MVT::i32, Expand);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Expand);

  // Shifts
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::ROTL, MVT::i32, Expand);
  setOperationAction(ISD::ROTR, MVT::i32, Expand);

  // Bit operations
  setOperationAction(ISD::BSWAP, MVT::i32, Expand);
  setOperationAction(ISD::CTTZ, MVT::i32, Expand);
  setOperationAction(ISD::CTLZ, MVT::i32, Expand);
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);

  // Global addresses
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);
  setOperationAction(ISD::BlockAddress, MVT::i32, Custom);
  setOperationAction(ISD::ConstantPool, MVT::i32, Custom);
  setOperationAction(ISD::JumpTable, MVT::i32, Custom);

  // Varargs
  setOperationAction(ISD::VASTART, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);

  // Dynamic stack allocation
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);

  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));

  setBooleanContents(ZeroOrOneBooleanContent);
  setMaxAtomicSizeInBitsSupported(0);
}

const char *TinyDSPTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  default:
    return nullptr;
  case TinyDSPISD::RET:
    return "TinyDSPISD::RET";
  case TinyDSPISD::CALL:
    return "TinyDSPISD::CALL";
  case TinyDSPISD::Wrapper:
    return "TinyDSPISD::Wrapper";
  }
}

SDValue TinyDSPTargetLowering::LowerOperation(SDValue Op,
                                              SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

SDValue TinyDSPTargetLowering::LowerGlobalAddress(SDValue Op,
                                                   SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();

  SDValue TargetAddr =
      DAG.getTargetGlobalAddress(GV, DL, MVT::i32, Offset);
  return DAG.getNode(TinyDSPISD::Wrapper, DL, MVT::i32, TargetAddr);
}

#include "TinyDSPGenCallingConv.inc"

SDValue TinyDSPTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_TinyDSP);

  for (auto &VA : ArgLocs) {
    if (VA.isRegLoc()) {
      // Arguments passed in registers
      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC = &TinyDSP::GPRRegClass;

      Register VReg = MF.getRegInfo().createVirtualRegister(RC);
      MF.getRegInfo().addLiveIn(VA.getLocReg(), VReg);
      SDValue ArgIn = DAG.getCopyFromReg(Chain, DL, VReg, RegVT);

      InVals.push_back(ArgIn);
    } else {
      // Arguments passed on stack
      assert(VA.isMemLoc() && "Expected memory location");
      
      int FI = MFI.CreateFixedObject(VA.getLocVT().getSizeInBits() / 8,
                                     VA.getLocMemOffset(), true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      InVals.push_back(
          DAG.getLoad(VA.getLocVT(), DL, Chain, FIN,
                      MachinePointerInfo::getFixedStack(MF, FI)));
    }
  }

  return Chain;
}

SDValue
TinyDSPTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                   bool isVarArg,
                                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                                   const SmallVectorImpl<SDValue> &OutVals,
                                   const SDLoc &DL, SelectionDAG &DAG) const {

  // CCValAssign - represent the assignment of the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  // Analyze return values.
  CCInfo.AnalyzeReturn(Outs, RetCC_TinyDSP);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[i], Glue);

    // Guarantee that all emitted copies are stuck together
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain; // Update chain

  // Add the glue if we have it.
  if (Glue.getNode())
    RetOps.push_back(Glue);

  return DAG.getNode(TinyDSPISD::RET, DL, MVT::Other, RetOps);
}

SDValue TinyDSPTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                         SmallVectorImpl<SDValue> &InVals) const {
  // TinyDSP target does not yet support tail call optimization.
  CLI.IsTailCall = false;

  // For now, only support simple direct calls
  return TargetLowering::LowerCall(CLI, InVals);
}
