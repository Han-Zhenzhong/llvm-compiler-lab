// RUN: %clang_cc1 "-triple" "nvptx64-nvidia-cuda" -emit-llvm -fcuda-is-device -o - %s | FileCheck %s

// CHECK: define{{.*}} void @_Z6kernelPf(ptr noundef %out)
__attribute__((global)) void kernel(float *out) {
  float x = 3.0f;
  out[0] = __nvvm_warp_reduce_add(x);
  // CHECK: call contract float @llvm.nvvm.warp.reduce.add
}
