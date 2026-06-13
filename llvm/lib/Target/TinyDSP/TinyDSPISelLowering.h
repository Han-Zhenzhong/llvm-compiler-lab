//===- TinyDSPISelLowering.h - TinyDSP DAG Lowering Interface --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that TinyDSP uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TINYDSP_TINYDPISELLOWERING_H
#define LLVM_LIB_TARGET_TINYDSP_TINYDPISELLOWERING_H

#include "TinyDSP.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class TinyDSPSubtarget;

class TinyDSPTargetLowering : public TargetLowering {
public:
  explicit TinyDSPTargetLowering(const TargetMachine &TM,
                                 const TinyDSPSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

private:
  const TinyDSPSubtarget &Subtarget;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                     const SmallVectorImpl<ISD::OutputArg> &Outs,
                     const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                     SelectionDAG &DAG) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                   SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_TINYDSP_TINYDPISELLOWERING_H
