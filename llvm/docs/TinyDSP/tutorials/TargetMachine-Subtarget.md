# Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                    LLVM CodeGen Pipeline                    │
└─────────────────────────────────────────────────────────────┘

LLVM IR Module
    │
    │ target triple = tinydsp
    v
┌─────────────────────────────────────────────────────────────┐
│ TargetRegistry                                               │
│                                                             │
│ 根据 triple / target name 找到 TinyDSP backend                │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPTargetMachine                                         │
│                                                             │
│ 代表“整个 TinyDSP 目标机器”                                  │
│                                                             │
│ 负责：                                                       │
│ - DataLayout                                                 │
│ - Relocation model                                           │
│ - Code model                                                 │
│ - Optimization level                                         │
│ - Pass pipeline configuration                                │
│ - 创建 TinyDSPSubtarget                                      │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPSubtarget                                             │
│                                                             │
│ 代表“某个具体 TinyDSP CPU / feature 组合”                     │
│                                                             │
│ 负责：                                                       │
│ - CPU variant                                                │
│ - target features                                            │
│ - scheduling model                                           │
│ - InstrInfo                                                  │
│ - RegisterInfo                                               │
│ - FrameLowering                                              │
│ - TargetLowering                                             │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ Target-specific CodeGen Components                          │
│                                                             │
│ TinyDSPInstrInfo                                             │
│ TinyDSPRegisterInfo                                          │
│ TinyDSPFrameLowering                                         │
│ TinyDSPISelLowering                                          │
│ TinyDSPSelectionDAGInfo                                      │
└─────────────────────────────────────────────────────────────┘
```

Simple understanding:

```text
TargetMachine = 目标机器级别的总入口
Subtarget     = 具体 CPU / feature / 指令能力的配置对象
```

# Relationship between TargetMachine/Subtarget and other TinyDSP backend implementations

```text
┌──────────────────────────────┐
│ TinyDSPTargetMachine          │
├──────────────────────────────┤
│ “我是 TinyDSP 这个 target”     │
│                              │
│ - 选择 CodeGen pipeline       │
│ - 定义 DataLayout             │
│ - 管理 Subtarget              │
│ - 创建 pass config            │
└───────────────┬──────────────┘
                │ owns / creates
                v
┌──────────────────────────────┐
│ TinyDSPSubtarget              │
├──────────────────────────────┤
│ “我是 TinyDSP 的某个具体变体”  │
│                              │
│ - 是否支持某些指令             │
│ - 使用哪个调度模型             │
│ - 有哪些寄存器/指令信息        │
│ - 如何 lowering / frame       │
└───────────────┬──────────────┘
                │ provides
                v
┌──────────────────────────────┐
│ Backend helper objects        │
├──────────────────────────────┤
│ TinyDSPInstrInfo              │
│ TinyDSPRegisterInfo           │
│ TinyDSPFrameLowering          │
│ TinyDSPTargetLowering         │
│ TinyDSPSelectionDAGInfo       │
└──────────────────────────────┘
```

Implementations file list:

```text
llvm/lib/Target/TinyDSP/
├── TinyDSPTargetMachine.h
├── TinyDSPTargetMachine.cpp
│
├── TinyDSPSubtarget.h
├── TinyDSPSubtarget.cpp
│
├── TinyDSPInstrInfo.h/.cpp
├── TinyDSPRegisterInfo.h/.cpp
├── TinyDSPFrameLowering.h/.cpp
├── TinyDSPISelLowering.h/.cpp
└── TinyDSPISelDAGToDAG.cpp
```

# Calling stack

```text
llc -mtriple=tinydsp input.ll
    │
    v
TargetRegistry::lookupTarget(...)
    │
    v
create TinyDSPTargetMachine
    │
    v
TinyDSPTargetMachine::getSubtargetImpl(Function &F)
    │
    v
TinyDSPSubtarget
    │
    ├── getInstrInfo()
    │       └── TinyDSPInstrInfo
    │
    ├── getRegisterInfo()
    │       └── TinyDSPRegisterInfo
    │
    ├── getFrameLowering()
    │       └── TinyDSPFrameLowering
    │
    └── getTargetLowering()
            └── TinyDSPTargetLowering
```

在 LLVM 后端里，很多 pass 不会直接问 `TinyDSPTargetMachine`：

```text
“这个指令怎么 copy physical register？”
“这个函数用哪个 frame pointer？”
“这个 target 有哪些 callee-saved registers？”
“这个 add 能不能合法 lowering？”
```

它们更多是通过 `Subtarget` 去拿对应组件：

```text
MachineFunction
    │
    v
Subtarget
    │
    ├── InstrInfo
    ├── RegisterInfo
    ├── FrameLowering
    └── TargetLowering
```

也就是：

```text
TargetMachine 更像“总配置中心”
Subtarget 更像“每个函数实际使用的 target-specific backend context”
```

为什么要有 `Subtarget`？

因为真实 CPU target 往往不是只有一个固定形态。例如：

```text
ARM
├── armv7
├── armv8
├── cortex-a53
├── cortex-a72
└── different feature sets

X86
├── x86-64
├── has SSE
├── has AVX
├── has AVX2
└── has AVX512
```

同一个 LLVM backend 里，不同 CPU / feature 可能有不同指令、不同调度模型、不同 lowering 策略。

所以 LLVM 把它拆成：

```text
TargetMachine
    代表整个 backend

Subtarget
    代表某个具体 CPU + feature combination
```

对 TinyDSP 来说，目前只有一个简单配置：

```text
TinyDSP
└── generic TinyDSP CPU
```

但保留 `Subtarget` 结构是必要的，因为 LLVM backend 框架本身就按这个模型组织。

可以这样图示未来扩展：

```text
TinyDSPTargetMachine
    │
    ├── TinyDSPSubtarget: generic
    │       ├── basic ADD/SUB/MUL/LOAD/STORE
    │       └── simple scheduling model
    │
    ├── TinyDSPSubtarget: tinydsp-v2
    │       ├── adds MAC instruction
    │       ├── adds vector registers
    │       └── different scheduling model
    │
    └── TinyDSPSubtarget: tinydsp-fastmul
            ├── faster MUL latency
            └── different instruction scheduling cost
```

# Overview detail

```text
┌─────────────────────────────────────────────┐
│ TinyDSPTargetMachine                         │
│                                             │
│ target-level object                          │
│ created once for TinyDSP compilation         │
│                                             │
│ answers:                                    │
│ “How should LLVM compile for TinyDSP?”       │
└───────────────────┬─────────────────────────┘
                    │
                    │ creates / owns
                    v
┌─────────────────────────────────────────────┐
│ TinyDSPSubtarget                             │
│                                             │
│ function-level target configuration          │
│ CPU + features + scheduling + backend infos  │
│                                             │
│ answers:                                    │
│ “For this function, what exact TinyDSP        │
│  variant and backend rules should be used?”  │
└───────────────────┬─────────────────────────┘
                    │
                    │ provides
                    v
┌─────────────────────────────────────────────┐
│ TinyDSP backend components                   │
│                                             │
│ InstrInfo        → instruction behavior      │
│ RegisterInfo     → physical registers        │
│ FrameLowering    → stack frame               │
│ ISelLowering     → IR/DAG lowering           │
│ SelectionDAGInfo → DAG-related target hooks  │
└─────────────────────────────────────────────┘
```

一句话总结：

> `TinyDSPTargetMachine` is the top-level target object used by LLVM to compile for TinyDSP, while `TinyDSPSubtarget` describes the concrete TinyDSP CPU/features used for a function and provides target-specific components such as instruction info, register info, frame lowering, and instruction lowering.

中文一句话：

> `TinyDSPTargetMachine` 是 TinyDSP 后端的总入口，负责目标机器级配置和 CodeGen pipeline；`TinyDSPSubtarget` 是具体 CPU/feature 级配置，向各个 CodeGen pass 提供指令、寄存器、栈帧和 lowering 等 target-specific 信息。
