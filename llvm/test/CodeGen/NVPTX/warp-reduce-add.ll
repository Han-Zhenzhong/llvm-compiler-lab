; RUN: llc < %s -mtriple=nvptx64 -mcpu=sm_70 | FileCheck %s

; This is a custom ProtoGPU instruction, so we do not run ptxas verification.
declare float @llvm.nvvm.warp.reduce.add(float)

; CHECK-LABEL: .func{{.*}}warp_reduce_add
; CHECK: warp_reduce_add.f32
; CHECK: ret;
define float @warp_reduce_add(float %src) {
  %val = call float @llvm.nvvm.warp.reduce.add(float %src)
  ret float %val
}
