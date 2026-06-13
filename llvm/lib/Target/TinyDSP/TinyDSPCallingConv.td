//===- TinyDSPCallingConv.td - Calling Conventions TinyDSP -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the TinyDSP architecture.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Return Value Calling Convention
//===----------------------------------------------------------------------===//

// TinyDSP return values:
// - i32 return values use R0
def RetCC_TinyDSP : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R0, R1]>>
]>;

//===----------------------------------------------------------------------===//
// TinyDSP Argument Calling Convention
//===----------------------------------------------------------------------===//

// TinyDSP calling convention:
// - First 4 i32 arguments are passed in R2, R3, R4, R5
// - Remaining arguments are passed on the stack
def CC_TinyDSP : CallingConv<[
  // Promote i8/i16 arguments to i32
  CCIfType<[i8, i16], CCPromoteToType<i32>>,
  
  // The first 4 integer arguments are passed in registers
  CCIfType<[i32], CCAssignToReg<[R2, R3, R4, R5]>>,
  
  // Integer values are assigned to the stack in 4-byte aligned units
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def CSR : CalleeSavedRegs<(add R6, R7)>;
