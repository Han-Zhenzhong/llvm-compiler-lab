//===- TinyDSPInstrFormats.td - TinyDSP Instruction Formats -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the TinyDSP instruction formats.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

class TinyDSPInst<dag outs, dag ins, string asmstr, list<dag> pattern>
  : Instruction {
  field bits<32> Inst;
  field bits<32> SoftFail = 0;

  let Namespace = "TinyDSP";
  let Size = 4;
  
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

//===----------------------------------------------------------------------===//
// Instruction Formats
//===----------------------------------------------------------------------===//

// R-Type: Register-Register operations
// Format: opcode(8) | rd(3) | rs1(3) | rs2(3) | unused(15)
class RType<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : TinyDSPInst<outs, ins, asmstr, pattern> {
  bits<3> rd;
  bits<3> rs1;
  bits<3> rs2;
  
  let Inst{31-24} = opcode;
  let Inst{23-21} = rd;
  let Inst{20-18} = rs1;
  let Inst{17-15} = rs2;
  let Inst{14-0}  = 0;
}

// I-Type: Immediate operations
// Format: opcode(8) | rd(3) | rs1(3) | imm(18)
class IType<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : TinyDSPInst<outs, ins, asmstr, pattern> {
  bits<3> rd;
  bits<3> rs1;
  bits<18> imm;
  
  let Inst{31-24} = opcode;
  let Inst{23-21} = rd;
  let Inst{20-18} = rs1;
  let Inst{17-0}  = imm;
}

// M-Type: Memory operations (Load/Store)
// Format: opcode(8) | rd/rs(3) | base(3) | offset(18)
class MType<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : TinyDSPInst<outs, ins, asmstr, pattern> {
  bits<3> reg;
  bits<3> base;
  bits<18> offset;
  
  let Inst{31-24} = opcode;
  let Inst{23-21} = reg;
  let Inst{20-18} = base;
  let Inst{17-0}  = offset;
}
