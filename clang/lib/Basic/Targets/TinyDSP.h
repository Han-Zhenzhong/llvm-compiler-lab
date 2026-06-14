//===--- TinyDSP.h - Declare TinyDSP target feature support ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares TinyDSP TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_TINYDSP_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_TINYDSP_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY TinyDSPTargetInfo : public TargetInfo {
  static const char *const GCCRegNames[];

public:
  TinyDSPTargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    BigEndian = false;
    TLSSupported = false;

    PointerWidth = PointerAlign = 32;
    IntWidth = IntAlign = 32;
    LongWidth = LongAlign = 32;
    LongLongWidth = LongLongAlign = 64;
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    IntMaxType = SignedLongLong;
    SuitableAlign = 32;

    resetDataLayout("e-m:e-p:32:32-i64:64-n32-S32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  bool hasFeature(StringRef Feature) const override {
    return Feature == "tinydsp";
  }

  bool isValidCPUName(StringRef Name) const override { return Name == "generic"; }

  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override {
    Values.emplace_back("generic");
  }

  bool setCPU(const std::string &Name) override { return isValidCPUName(Name); }

  ArrayRef<const char *> getGCCRegNames() const override;

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    static const TargetInfo::GCCRegAlias GCCRegAliases[] = {
        {{"sp"}, "r7"},
        {{"fp"}, "r6"},
    };
    return llvm::ArrayRef(GCCRegAliases);
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    (void)Name;
    (void)Info;
    return false;
  }

  std::string_view getClobbers() const override { return ""; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_TINYDSP_H
