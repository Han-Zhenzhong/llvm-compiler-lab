//===- TinyDSPMCAsmInfo.cpp - TinyDSP Asm Properties ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the TinyDSPMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "TinyDSPMCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

TinyDSPMCAsmInfo::TinyDSPMCAsmInfo(const Triple &TT,
                                   const MCTargetOptions &Options) {
  IsLittleEndian = true;
  
  AlignmentIsInBytes = false;
  Data16bitsDirective = "\t.2byte\t";
  Data32bitsDirective = "\t.4byte\t";
  Data64bitsDirective = "\t.8byte\t";
  PrivateLabelPrefix = ".L";
  CommentString = "#";
  
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
