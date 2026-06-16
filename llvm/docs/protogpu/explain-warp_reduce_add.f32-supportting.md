**NVPTX / ProtoGPU 自定义指令实验**：

```text
commit 955ffbe
[NVPTX] Add warp_reduce_add.f32 custom builtin/intrinsic flow
```

它做的事情是：给 Clang/LLVM/NVPTX 增加一条从 **CUDA builtin → LLVM NVVM intrinsic → NVPTX 指令选择 → PTX 自定义 opcode** 的完整通路。commit 页面显示它改了 7 个文件，新增 144 行，包括 Clang builtin、LLVM NVVM intrinsic、NVPTX 指令 pattern、LLVM/Clang 测试和构建说明文档。([GitHub][1])

---

## 1. 一张总图

```text
CUDA device code
    │
    │ call __nvvm_warp_reduce_add(x)
    v
┌─────────────────────────────────────────────┐
│ Clang builtin layer                          │
│ clang/include/clang/Basic/BuiltinsNVPTX.td  │
│                                             │
│ 新增 builtin: __nvvm_warp_reduce_add         │
└──────────────────────┬──────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────┐
│ LLVM IR intrinsic layer                      │
│ llvm/include/llvm/IR/IntrinsicsNVVM.td       │
│                                             │
│ 新增 intrinsic: llvm.nvvm.warp.reduce.add    │
└──────────────────────┬──────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────┐
│ NVPTX backend instruction selection          │
│ llvm/lib/Target/NVPTX/NVPTXIntrinsics.td     │
│                                             │
│ intrinsic → warp_reduce_add.f32              │
└──────────────────────┬──────────────────────┘
                       │
                       v
PTX output
    │
    v
warp_reduce_add.f32
```

一句话：

```text
这个 commit 打通了：
CUDA 源码中的自定义 builtin
    ↓
LLVM IR 中的自定义 NVVM intrinsic
    ↓
NVPTX 后端中的 TableGen 指令匹配
    ↓
最终 PTX 文本里的自定义指令 warp_reduce_add.f32
```

---

## 2. 改动 1：Clang 认识新的 builtin

文件：

```text
clang/include/clang/Basic/BuiltinsNVPTX.td
```

新增：

```text
def __nvvm_warp_reduce_add : NVPTXBuiltin<"float(float)">;
```

commit 里注释为 `ProtoGPU custom instruction: warp_reduce_add.f32 dst, src`。([GitHub][1])

含义：

```text
让 Clang 认识这个函数名：

__nvvm_warp_reduce_add(float) -> float
```

图示：

```text
CUDA source
  │
  │ __nvvm_warp_reduce_add(x)
  v
Clang builtin table
  │
  │ 发现这是 NVPTX builtin
  v
生成对应 LLVM intrinsic call
```

没有这一步，Clang 看到：

```c
__nvvm_warp_reduce_add(x)
```

可能就只是普通未声明函数，或者直接报错，无法走 NVVM intrinsic 路径。

---

## 3. 改动 2：LLVM IR 认识新的 NVVM intrinsic

文件：

```text
llvm/include/llvm/IR/IntrinsicsNVVM.td
```

新增：

```text
def int_nvvm_warp_reduce_add : NVVMBuiltin,
    Intrinsic<[llvm_float_ty], [llvm_float_ty]>;
```

也就是定义一个 LLVM intrinsic：

```llvm
declare float @llvm.nvvm.warp.reduce.add(float)
```

commit 页面显示该 intrinsic 的注释是 `warp_reduce_add.f32 dst, src;`，类型是输入一个 `float`，返回一个 `float`。([GitHub][1])

图示：

```text
Clang builtin
  │
  v
LLVM IR intrinsic
  │
  v
call float @llvm.nvvm.warp.reduce.add(float %x)
```

这一层的意义是：

```text
把“前端函数调用”
变成
LLVM 中间表示里有明确语义的 intrinsic call
```

---

## 4. 改动 3：NVPTX 后端把 intrinsic 选成 PTX 指令

文件：

```text
llvm/lib/Target/NVPTX/NVPTXIntrinsics.td
```

新增：

```text
def WARP_REDUCE_ADD_F32 :
  BasicNVPTXInst<(outs B32:$dst), (ins B32:$src),
                 "warp_reduce_add.f32",
                 [(set f32:$dst, (int_nvvm_warp_reduce_add f32:$src))]>;
```

commit 页面显示这个 pattern 把 `int_nvvm_warp_reduce_add` 匹配成 PTX mnemonic `warp_reduce_add.f32`。([GitHub][1])

图示：

```text
LLVM IR:
  %val = call float @llvm.nvvm.warp.reduce.add(float %src)

SelectionDAG / NVPTX ISel:
  int_nvvm_warp_reduce_add f32:$src
      ↓ TableGen pattern
  WARP_REDUCE_ADD_F32

PTX:
  warp_reduce_add.f32
```

这一步是最核心的后端改动。

简单说：

```text
Intrinsic 是 LLVM IR 层的名字。
warp_reduce_add.f32 是 PTX 层的指令文本。
NVPTXIntrinsics.td 把二者连起来。
```

---

## 5. 改动 4：Clang CodeGen 测试

文件：

```text
clang/test/CodeGenCUDA/warp-reduce-add-builtin.cu
```

测试代码大概是：

```cuda
__attribute__((global)) void kernel(float *out) {
  float x = 3.0f;
  out[0] = __nvvm_warp_reduce_add(x);
}
```

检查点：

```text
CHECK: call contract float @llvm.nvvm.warp.reduce.add
```

commit 页面显示这个测试用 `clang_cc1` 对 CUDA device code emit LLVM IR，并检查是否生成了 `llvm.nvvm.warp.reduce.add` intrinsic call。([GitHub][1])

图示：

```text
warp-reduce-add-builtin.cu
  │
  │ clang_cc1 -emit-llvm -fcuda-is-device
  v
LLVM IR
  │
  │ FileCheck
  v
确认出现：
  call float @llvm.nvvm.warp.reduce.add
```

这个测试验证的是：

```text
Clang builtin → LLVM intrinsic
```

---

## 6. 改动 5：LLVM NVPTX CodeGen 测试

文件：

```text
llvm/test/CodeGen/NVPTX/warp-reduce-add.ll
```

测试 IR：

```llvm
declare float @llvm.nvvm.warp.reduce.add(float)

define float @warp_reduce_add(float %src) {
  %val = call float @llvm.nvvm.warp.reduce.add(float %src)
  ret float %val
}
```

测试命令：

```text
llc < %s -mtriple=nvptx64 -mcpu=sm_70 | FileCheck %s
```

检查输出包含：

```text
warp_reduce_add.f32
ret;
```

commit 页面也注明：这是 ProtoGPU custom instruction，所以不运行 `ptxas` verification。([GitHub][1])

图示：

```text
LLVM IR intrinsic
  │
  │ llc -mtriple=nvptx64 -mcpu=sm_70
  v
PTX assembly
  │
  │ FileCheck
  v
确认出现：
  warp_reduce_add.f32
```

这个测试验证的是：

```text
LLVM intrinsic → NVPTX PTX opcode
```

---

## 7. 改动 6：文档说明 custom clang 如何构建

文件：

```text
llvm/docs/HowToBuildProtoGPUCustomClang.rst
llvm/docs/UserGuides.rst
```

文档说明了如何构建支持这个自定义 NVPTX 指令的 Clang/LLVM，要求启用 `NVPTX` target，例如：

```bash
cmake -S llvm -B build-nvptx -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD="X86;NVPTX"

ninja -C build-nvptx clang llc
```

commit 页面里还写了验证命令：用 `llc` 编译 `llvm/test/CodeGen/NVPTX/warp-reduce-add.ll`，预期输出包含 `warp_reduce_add.f32`。([GitHub][1])

文档也强调一个重要限制：

```text
__nvvm_warp_reduce_add 是 compiler builtin / NVVM intrinsic 入口，
不是 PTX 指令 mnemonic。

真正 emit 出来的 PTX 指令是 warp_reduce_add.f32。
```

并且 NVIDIA `ptxas` 可能拒绝这种 custom opcode，因为下游 CUDA assembler/JIT 不认识这个自定义指令；文档建议 ProtoGPU 走 split flow。([GitHub][1])

---

## 8. 最关键的逻辑链路

```text
用户写 CUDA:
  out[0] = __nvvm_warp_reduce_add(x);

        ↓

Clang builtin table:
  __nvvm_warp_reduce_add : float(float)

        ↓

LLVM IR:
  call float @llvm.nvvm.warp.reduce.add(float %x)

        ↓

NVPTX TableGen pattern:
  int_nvvm_warp_reduce_add f32:$src
      -> WARP_REDUCE_ADD_F32

        ↓

PTX:
  warp_reduce_add.f32
```

再压缩一下：

```text
builtin name
  ↓
LLVM intrinsic
  ↓
NVPTX instruction pattern
  ↓
custom PTX opcode
```

---

## 9. 这个 commit 的工程价值

帮助理解 **compiler frontend → LLVM IR → backend instruction selection → target assembly** 的完整链路。

```text
┌──────────────────────────┐
│ Frontend extension        │
│ Clang builtin             │
└─────────────┬────────────┘
              v
┌──────────────────────────┐
│ IR extension              │
│ LLVM NVVM intrinsic       │
└─────────────┬────────────┘
              v
┌──────────────────────────┐
│ Backend lowering          │
│ NVPTX TableGen pattern    │
└─────────────┬────────────┘
              v
┌──────────────────────────┐
│ Target output             │
│ custom PTX instruction    │
└─────────────┬────────────┘
              v
┌──────────────────────────┐
│ Verification              │
│ Clang + LLVM FileCheck    │
└──────────────────────────┘
```

Summary：

> This branch added a custom NVPTX compiler path for a ProtoGPU instruction. The flow starts from a Clang NVPTX builtin, lowers to a new LLVM NVVM intrinsic, and is selected by the NVPTX backend into a custom PTX opcode `warp_reduce_add.f32`. It also added Clang and LLVM CodeGen tests and documented the custom toolchain build flow.

中文：

> 该分支实现了一条 ProtoGPU 自定义 PTX 指令的编译器路径：从 Clang NVPTX builtin 开始， lowering 到新的 LLVM NVVM intrinsic，再由 NVPTX 后端 TableGen pattern 选择成自定义 PTX opcode `warp_reduce_add.f32`。同时补了 Clang/LLVM CodeGen 测试和 custom clang 构建说明。

---

## 10. 最简单的理解

```text
这次 commit 做的不是：
  “在 C++ 里手写一个函数”。

它做的是：
  “让编译器真正认识一个新操作，
   并能从 CUDA 源码一路编译到 PTX 自定义指令。”
```

最小图：

```text
__nvvm_warp_reduce_add(x)
        ↓
@llvm.nvvm.warp.reduce.add(x)
        ↓
warp_reduce_add.f32
```

[1]: https://github.com/Han-Zhenzhong/llvm-compiler-lab/commit/955ffbeeea464532dde05e469f52d25bab212f61 "[NVPTX] Add warp_reduce_add.f32 custom builtin/intrinsic flow · Han-Zhenzhong/llvm-compiler-lab@955ffbe · GitHub"
