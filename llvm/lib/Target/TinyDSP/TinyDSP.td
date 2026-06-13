//===- TinyDSP.td - Describe the TinyDSP Target Machine ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is the top level entry point for the TinyDSP target.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "TinyDSPRegisterInfo.td"
include "TinyDSPCallingConv.td"
include "TinyDSPSchedule.td"
include "TinyDSPInstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def TinyDSPInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// TinyDSP processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic", TinyDSPSchedModel, []>;

def TinyDSPInstPrinter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def TinyDSP : Target {
  let InstructionSet = TinyDSPInstrInfo;
  let AssemblyWriters = [TinyDSPInstPrinter];
}
