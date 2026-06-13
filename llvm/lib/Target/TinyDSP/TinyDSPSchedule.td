//===- TinyDSPSchedule.td - TinyDSP Scheduling Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the scheduling model for TinyDSP.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// TinyDSP Scheduling Model
//===----------------------------------------------------------------------===//

def TinyDSPSchedModel : SchedMachineModel {
  let IssueWidth = 1;         // Single issue processor
  let MicroOpBufferSize = 4;  // Small buffer
  let LoadLatency = 2;        // Load takes 2 cycles
  let HighLatency = 10;       // High latency threshold
  let PostRAScheduler = 0;    // Disable post-RA scheduling for simplicity
  let CompleteModel = 0;
}

//===----------------------------------------------------------------------===//
// Define functional units
//===----------------------------------------------------------------------===//

def TinyDSP_ALU : ProcResource<1>;    // Arithmetic Logic Unit
def TinyDSP_MUL : ProcResource<1>;    // Multiplier
def TinyDSP_LSU : ProcResource<1>;    // Load/Store Unit

//===----------------------------------------------------------------------===//
// Instruction Itineraries
//===----------------------------------------------------------------------===//

def ALU_Op : SchedWrite;
def MUL_Op : SchedWrite;
def LD_Op  : SchedWrite;
def ST_Op  : SchedWrite;

let SchedModel = TinyDSPSchedModel in {
def : WriteRes<ALU_Op, [TinyDSP_ALU]> { let Latency = 1; }
def : WriteRes<MUL_Op, [TinyDSP_MUL]> { let Latency = 3; }
def : WriteRes<LD_Op,  [TinyDSP_LSU]> { let Latency = 2; }
def : WriteRes<ST_Op,  [TinyDSP_LSU]> { let Latency = 1; }
}
