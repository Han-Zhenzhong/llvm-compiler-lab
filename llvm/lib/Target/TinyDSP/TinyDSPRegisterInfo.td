//===- TinyDSPRegisterInfo.td - TinyDSP Register defs -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations that describe the TinyDSP register file
//
//===----------------------------------------------------------------------===//

// TinyDSP registers are identified with 3-bit ID numbers (R0-R7)
class TinyDSPReg<bits<16> Enc, string n, list<string> altNames = []> 
  : Register<n, altNames> {
  let HWEncoding = Enc;
  let Namespace = "TinyDSP";
}

// General Purpose Registers - 8 registers: R0 to R7
def R0 : TinyDSPReg<0, "r0">, DwarfRegNum<[0]>;
def R1 : TinyDSPReg<1, "r1">, DwarfRegNum<[1]>;
def R2 : TinyDSPReg<2, "r2">, DwarfRegNum<[2]>;
def R3 : TinyDSPReg<3, "r3">, DwarfRegNum<[3]>;
def R4 : TinyDSPReg<4, "r4">, DwarfRegNum<[4]>;
def R5 : TinyDSPReg<5, "r5">, DwarfRegNum<[5]>;
def R6 : TinyDSPReg<6, "r6">, DwarfRegNum<[6]>;
def R7 : TinyDSPReg<7, "r7">, DwarfRegNum<[7]>;

// Register class for general purpose registers
// R0-R1: return values and temporaries
// R2-R5: arguments and temporaries
// R6: frame pointer
// R7: stack pointer
def GPR : RegisterClass<"TinyDSP", [i32], 32, (add
  // Allocate in this order to preserve R7 (SP) and R6 (FP)
  R0, R1, R2, R3, R4, R5,
  R6,  // Frame pointer
  R7   // Stack pointer
)>;
