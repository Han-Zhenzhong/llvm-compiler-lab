//===- TinyDSPInstrInfo.td - TinyDSP Instruction defs -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the TinyDSP instruction set.
//
//===----------------------------------------------------------------------===//

include "TinyDSPInstrFormats.td"

//===----------------------------------------------------------------------===//
// Custom SDNodes
//===----------------------------------------------------------------------===//

def SDT_TinyDSPRet : SDTypeProfile<0, 0, []>;

def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>,
                                       SDTCisVT<1, i32>]>;
def SDT_CallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i32>,
                                   SDTCisVT<1, i32>]>;

def TinyDSPRet : SDNode<"TinyDSPISD::RET", SDT_TinyDSPRet,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd,
                         [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

//===----------------------------------------------------------------------===//
// Operand Definitions
//===----------------------------------------------------------------------===//

def simm18 : Operand<i32>, ImmLeaf<i32, [{
  return isInt<18>(Imm);
}]>;

def MEMri : Operand<iPTR> {
  let MIOperandInfo = (ops GPR, simm18);
  let PrintMethod = "printMemOperand";
  let EncoderMethod = "encodeMemoryOpValue";
}

//===----------------------------------------------------------------------===//
// Instruction Definitions
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {

// ADD: rd = rs1 + rs2
def ADD : RType<0x01, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "add\t$rd, $rs1, $rs2",
                [(set i32:$rd, (add i32:$rs1, i32:$rs2))]> {
  let SchedRW = [ALU_Op];
}

// SUB: rd = rs1 - rs2
def SUB : RType<0x02, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "sub\t$rd, $rs1, $rs2",
                [(set i32:$rd, (sub i32:$rs1, i32:$rs2))]> {
  let SchedRW = [ALU_Op];
}

// MUL: rd = rs1 * rs2
def MUL : RType<0x03, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "mul\t$rd, $rs1, $rs2",
                [(set i32:$rd, (mul i32:$rs1, i32:$rs2))]> {
  let SchedRW = [MUL_Op];
}

// AND: rd = rs1 & rs2
def AND : RType<0x04, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "and\t$rd, $rs1, $rs2",
                [(set i32:$rd, (and i32:$rs1, i32:$rs2))]> {
  let SchedRW = [ALU_Op];
}

// OR: rd = rs1 | rs2
def OR : RType<0x05, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
               "or\t$rd, $rs1, $rs2",
               [(set i32:$rd, (or i32:$rs1, i32:$rs2))]> {
  let SchedRW = [ALU_Op];
}

// XOR: rd = rs1 ^ rs2
def XOR : RType<0x06, (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "xor\t$rd, $rs1, $rs2",
                [(set i32:$rd, (xor i32:$rs1, i32:$rs2))]> {
  let SchedRW = [ALU_Op];
}

// ADDI: rd = rs1 + imm
def ADDI : IType<0x11, (outs GPR:$rd), (ins GPR:$rs1, simm18:$imm),
                 "addi\t$rd, $rs1, $imm",
                 [(set i32:$rd, (add i32:$rs1, simm18:$imm))]> {
  let SchedRW = [ALU_Op];
}

} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

// LOAD: rd = mem[rs1 + offset]
let mayLoad = 1, mayStore = 0, hasSideEffects = 0 in {
def LOAD : MType<0x20, (outs GPR:$reg), (ins GPR:$base, simm18:$offset),
                 "load\t$reg, $base, $offset",
                 []> {
  let SchedRW = [LD_Op];
}
}

// STORE: mem[base + offset] = reg
let mayLoad = 0, mayStore = 1, hasSideEffects = 0 in {
def STORE : MType<0x21, (outs), (ins GPR:$reg, GPR:$base, simm18:$offset),
                  "store\t$reg, $base, $offset",
                  []> {
  let SchedRW = [ST_Op];
}
}

// RET: Return from function
let isReturn = 1, isTerminator = 1, isBarrier = 1 in {
def RET : TinyDSPInst<(outs), (ins), "ret", [(TinyDSPRet)]> {
  let Inst{31-24} = 0x30;
  let Inst{23-0}  = 0;
}
}

//===----------------------------------------------------------------------===//
// Pseudo Instructions
//===----------------------------------------------------------------------===//

let Defs = [R7], Uses = [R7], hasSideEffects = 0 in {
def ADJCALLSTACKDOWN : TinyDSPInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                   "# ADJCALLSTACKDOWN $amt1 $amt2",
                                   [(callseq_start timm:$amt1, timm:$amt2)]>;

def ADJCALLSTACKUP : TinyDSPInst<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                 "# ADJCALLSTACKUP $amt1 $amt2",
                                 [(callseq_end timm:$amt1, timm:$amt2)]>;
}

// Move register to register (implemented as ADD with R0)
let isCodeGenOnly = 1, isPseudo = 1 in {
def MOV : TinyDSPInst<(outs GPR:$rd), (ins GPR:$rs),
                      "mov\t$rd, $rs", []>;
}

// Load immediate (for constants that don't fit in instruction immediate)
let isReMaterializable = 1, isCodeGenOnly = 1 in {
def LI : TinyDSPInst<(outs GPR:$rd), (ins i32imm:$imm),
                     "li\t$rd, $imm",
                     [(set i32:$rd, imm:$imm)]>;
}

//===----------------------------------------------------------------------===//
// Additional Pattern Matching
//===----------------------------------------------------------------------===//

// TODO: Add address-matching patterns after introducing a ComplexPattern for
// TinyDSP memory operands (base + simm18).
