
```text
llvm/lib/Target/TinyDSP/TinyDSP.td
```

它是 **TinyDSP 后端的 TableGen 顶层入口文件**。LLVM 构建时，`CMakeLists.txt` 里设置：

```cmake
set(LLVM_TARGET_DEFINITIONS TinyDSP.td)
```

所以后续 `tablegen(...)` 命令都会从 `TinyDSP.td` 开始读取 TinyDSP 的 target 描述。`TinyDSP.td` 文件本身只有 50 行左右，但它把寄存器、调用约定、调度模型、指令描述、汇编打印器和 Target 定义都串起来了。

```text
┌─────────────────────────────────────────────────────────────┐
│              llvm/lib/Target/TinyDSP/TinyDSP.td             │
│                                                             │
│      Top-level TableGen entry for the TinyDSP target         │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            │ include
                            v
┌─────────────────────────────────────────────────────────────┐
│ include "llvm/Target/Target.td"                             │
│                                                             │
│ 引入 LLVM target-independent TableGen 基类                   │
│                                                             │
│ 提供：                                                       │
│ - Target                                                     │
│ - InstrInfo                                                  │
│ - RegisterClass                                              │
│ - ProcessorModel                                             │
│ - AsmWriter                                                  │
│ - SDNode / Pat / instruction pattern related definitions     │
└───────────────────────────┬─────────────────────────────────┘
                            │
          ┌─────────────────┼──────────────────┐
          │                 │                  │
          v                 v                  v
┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│ Register Info  │ │ Calling Conv    │ │ Schedule Model │
│                │ │                │ │                │
│ TinyDSPRegister│ │ TinyDSPCalling │ │ TinyDSPSchedule│
│ Info.td        │ │ Conv.td        │ │ .td            │
└───────┬────────┘ └───────┬────────┘ └───────┬────────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           v
                 ┌──────────────────┐
                 │ Instruction Info │
                 │                  │
                 │ TinyDSPInstrInfo │
                 │ .td              │
                 └────────┬─────────┘
                          │
                          v
┌─────────────────────────────────────────────────────────────┐
│ def TinyDSPInstrInfo : InstrInfo;                           │
│                                                             │
│ 定义 TinyDSP 的 instruction set descriptor                   │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            v
┌─────────────────────────────────────────────────────────────┐
│ def : ProcessorModel<"generic", TinyDSPSchedModel, []>;     │
│                                                             │
│ 定义 TinyDSP 支持的 processor model: generic                 │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            v
┌─────────────────────────────────────────────────────────────┐
│ def TinyDSPInstPrinter : AsmWriter                          │
│                                                             │
│ 定义 TinyDSP 汇编打印器                                      │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            v
┌─────────────────────────────────────────────────────────────┐
│ def TinyDSP : Target                                        │
│                                                             │
│ let InstructionSet = TinyDSPInstrInfo;                      │
│ let AssemblyWriters = [TinyDSPInstPrinter];                 │
│                                                             │
│ 最终声明：这是一个 LLVM Target                               │
└─────────────────────────────────────────────────────────────┘
```

逐段看。

## 1. 引入 LLVM target-independent TableGen 基础定义

```tablegen
include "llvm/Target/Target.td"
```

这行的意思是：

```text
TinyDSP.td 不是从零定义 backend 语法。
它先引入 LLVM 已经定义好的 target 描述基础设施。
```

`Target.td` 里有很多 LLVM 后端通用的 TableGen 基类，例如：

```text
Target
InstrInfo
Register
RegisterClass
ProcessorModel
SchedMachineModel
AsmWriter
Instruction
SDPatternOperator
Pat
```

所以可以把它理解成：

```text
llvm/Target/Target.td
        ↓
提供 LLVM backend TableGen 语言中的“基础类库”
        ↓
TinyDSP.td 基于这些类描述 TinyDSP target
```

图示：

```text
┌──────────────────────────────┐
│ LLVM Target.td                │
├──────────────────────────────┤
│ Target                        │
│ InstrInfo                     │
│ Register                      │
│ RegisterClass                 │
│ ProcessorModel                │
│ AsmWriter                     │
│ Instruction                   │
└───────────────┬──────────────┘
                │ inherited / used by
                v
┌──────────────────────────────┐
│ TinyDSP TableGen files        │
├──────────────────────────────┤
│ TinyDSPRegisterInfo.td        │
│ TinyDSPInstrInfo.td           │
│ TinyDSPCallingConv.td         │
│ TinyDSPSchedule.td            │
│ TinyDSP.td                    │
└──────────────────────────────┘
```

## 2. 引入 TinyDSP 自己的 `.td` 文件

`TinyDSP.td` 接着 include 了这些文件：

```tablegen
include "TinyDSPRegisterInfo.td"
include "TinyDSPCallingConv.td"
include "TinyDSPSchedule.td"
include "TinyDSPInstrInfo.td"
```

它们分别负责：

```text
TinyDSPRegisterInfo.td
    定义 TinyDSP 物理寄存器和寄存器类，例如 GPR。

TinyDSPCallingConv.td
    定义 TinyDSP 的参数传递和返回值规则。

TinyDSPSchedule.td
    定义 TinyDSP 的调度模型，例如 generic processor 的指令延迟/资源模型。

TinyDSPInstrInfo.td
    定义 TinyDSP 指令、指令格式、SelectionDAG pattern。
```

整体关系：

```text
TinyDSP.td
    │
    ├── TinyDSPRegisterInfo.td
    │       └── registers / register classes
    │
    ├── TinyDSPCallingConv.td
    │       └── ABI / argument / return value rules
    │
    ├── TinyDSPSchedule.td
    │       └── scheduling model
    │
    └── TinyDSPInstrInfo.td
            └── instruction definitions and patterns
```

所以 `TinyDSP.td` 本身更像：

```text
总装配文件 / top-level manifest / target declaration file
```

真正大量细节在被 include 的 `.td` 文件里。

## 3. `RemapAllTargetPseudoPointerOperands<GPR>`

文件里有这一行：

```tablegen
defm : RemapAllTargetPseudoPointerOperands<GPR>;
```

可以先用工程视角理解：

```text
把 target pseudo instruction 中的 pointer-like operands 映射到 TinyDSP 的 GPR 寄存器类。
```

也就是告诉 LLVM：

```text
TinyDSP 的普通指针/地址类操作数，最终应该使用 GPR 这种寄存器类来承载。
```

图示：

```text
LLVM generic pseudo pointer operand
        │
        │ RemapAllTargetPseudoPointerOperands<GPR>
        v
TinyDSP GPR register class
        │
        v
R0 / R1 / R2 / ... / R7
```

这个点在小型 backend 里经常容易忽略。它不是定义一条具体指令，而是在 TableGen 层做一种“目标相关寄存器类映射”。

## 4. 定义 TinyDSP instruction set descriptor

```tablegen
def TinyDSPInstrInfo : InstrInfo;
```

这行定义了 TinyDSP 的 instruction set 信息对象。

它后面会被 `Target` 使用：

```tablegen
def TinyDSP : Target {
  let InstructionSet = TinyDSPInstrInfo;
  let AssemblyWriters = [TinyDSPInstPrinter];
}
```

可以理解为：

```text
TinyDSPInstrInfo
    是 TinyDSP target 的 instruction set 入口对象。
```

图示：

```text
TinyDSPInstrInfo.td
    │
    ├── ADD
    ├── SUB
    ├── MUL
    ├── LOAD
    ├── STORE
    └── RET
        │
        v
def TinyDSPInstrInfo : InstrInfo
        │
        v
def TinyDSP : Target {
    let InstructionSet = TinyDSPInstrInfo;
}
```

也就是说，最终 `Target` 不是直接把所有指令逐条列进去，而是通过 `InstructionSet = TinyDSPInstrInfo` 指向 TinyDSP 的指令集合入口。

## 5. 定义 processor model

```tablegen
def : ProcessorModel<"generic", TinyDSPSchedModel, []>;
```

这行定义 TinyDSP 支持的一个 processor model：

```text
CPU name: generic
Schedule model: TinyDSPSchedModel
Features: []
```

含义：

```text
TinyDSP 目前只有一个 generic CPU 变体。
它使用 TinyDSPSchedModel 作为调度模型。
目前没有额外 feature。
```

图示：

```text
┌──────────────────────────────────────┐
│ ProcessorModel<"generic", ...>       │
├──────────────────────────────────────┤
│ CPU name: generic                    │
│ Schedule model: TinyDSPSchedModel    │
│ Feature list: []                     │
└──────────────────────────────────────┘
```

它和 `Subtarget` 的关系是：

```text
llc -mcpu=generic -mtriple=tinydsp
        │
        v
ProcessorModel<"generic", TinyDSPSchedModel, []>
        │
        v
TinyDSPSubtarget
        │
        v
instruction scheduling / feature selection
```

未来如果你扩展 TinyDSP，可以这样加：

```tablegen
def : ProcessorModel<"tinydsp-v2", TinyDSPV2SchedModel, [FeatureMAC]>;
```

概念图：

```text
TinyDSP target
    │
    ├── generic
    │       ├── basic ALU
    │       └── TinyDSPSchedModel
    │
    └── tinydsp-v2
            ├── MAC instruction
            ├── vector extension
            └── TinyDSPV2SchedModel
```

## 6. 定义汇编打印器

```tablegen
def TinyDSPInstPrinter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}
```

这定义 TinyDSP 的 assembly writer。

含义：

```text
TinyDSPInstPrinter 是一个 AsmWriter。
它用于生成 TinyDSP 的汇编打印相关 TableGen 代码。
```

其中：

```text
AsmWriterClassName = "InstPrinter"
    生成的 asm writer 类名相关设置。

isMCAsmWriter = 1
    表示这是 MC layer 使用的 asm writer。
```

它和生成文件的关系：

```text
TinyDSPInstPrinter : AsmWriter
        │
        │ tablegen -gen-asm-writer
        v
TinyDSPGenAsmWriter.inc
        │
        v
TinyDSPInstPrinter.cpp / MCTargetDesc layer
        │
        v
MCInst → TinyDSP assembly text
```

图示：

```text
MCInst
  │
  v
TinyDSPInstPrinter
  │
  v
TinyDSP assembly

example:
  ADD R0, R2, R3
```

## 7. 最终声明 TinyDSP target

最重要的一段：

```tablegen
def TinyDSP : Target {
  let InstructionSet = TinyDSPInstrInfo;
  let AssemblyWriters = [TinyDSPInstPrinter];
}
```

这告诉 TableGen：

```text
TinyDSP 是一个 LLVM Target。
它的指令集入口是 TinyDSPInstrInfo。
它的汇编输出器是 TinyDSPInstPrinter。
```

图示：

```text
┌──────────────────────────────────────┐
│ def TinyDSP : Target                 │
├──────────────────────────────────────┤
│ InstructionSet                       │
│   └── TinyDSPInstrInfo               │
│                                      │
│ AssemblyWriters                      │
│   └── TinyDSPInstPrinter             │
└──────────────────────────────────────┘
```

这部分是整个 `.td` 文件的“最终出口”。

前面所有 include 和定义，最终都被挂到这个 `Target` 上：

```text
Registers
CallingConv
ScheduleModel
Instructions
AsmWriter
        │
        v
def TinyDSP : Target
        │
        v
LLVM TableGen generated backend metadata
```

## 8. 从 `TinyDSP.td` 到生成文件

因为 `CMakeLists.txt` 里会运行多种 `tablegen(...)`，所以 `TinyDSP.td` 会被不同 TableGen backend 读取多次，每次生成不同用途的 `.inc` 文件。

```text
TinyDSP.td
  │
  ├── tablegen -gen-register-info
  │       └── TinyDSPGenRegisterInfo.inc
  │
  ├── tablegen -gen-instr-info
  │       └── TinyDSPGenInstrInfo.inc
  │
  ├── tablegen -gen-dag-isel
  │       └── TinyDSPGenDAGISel.inc
  │
  ├── tablegen -gen-callingconv
  │       └── TinyDSPGenCallingConv.inc
  │
  ├── tablegen -gen-subtarget
  │       └── TinyDSPGenSubtargetInfo.inc
  │
  ├── tablegen -gen-asm-writer
  │       └── TinyDSPGenAsmWriter.inc
  │
  └── tablegen -gen-emitter
          └── TinyDSPGenMCCodeEmitter.inc
```

这些生成文件再被 C++ 文件 include：

```text
TinyDSPGenRegisterInfo.inc
    ↓
TinyDSPRegisterInfo.cpp

TinyDSPGenInstrInfo.inc
    ↓
TinyDSPInstrInfo.cpp

TinyDSPGenDAGISel.inc
    ↓
TinyDSPISelDAGToDAG.cpp

TinyDSPGenCallingConv.inc
    ↓
TinyDSPISelLowering.cpp

TinyDSPGenSubtargetInfo.inc
    ↓
TinyDSPSubtarget.cpp

TinyDSPGenAsmWriter.inc
    ↓
TinyDSPInstPrinter / AsmPrinter related code

TinyDSPGenMCCodeEmitter.inc
    ↓
TinyDSPMCCodeEmitter.cpp
```

## 9. 完整 mental model

```text
┌─────────────────────────────────────────────────────────────┐
│ TinyDSP.td                                                   │
│                                                             │
│ top-level TableGen entry                                     │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ├── include LLVM Target.td
                        │       └── get target-independent base classes
                        │
                        ├── include TinyDSPRegisterInfo.td
                        │       └── define registers and register classes
                        │
                        ├── include TinyDSPCallingConv.td
                        │       └── define ABI rules
                        │
                        ├── include TinyDSPSchedule.td
                        │       └── define scheduling model
                        │
                        ├── include TinyDSPInstrInfo.td
                        │       └── define instructions and patterns
                        │
                        ├── define TinyDSPInstrInfo : InstrInfo
                        │
                        ├── define ProcessorModel<"generic", ...>
                        │
                        ├── define TinyDSPInstPrinter : AsmWriter
                        │
                        └── define TinyDSP : Target
                                ├── InstructionSet = TinyDSPInstrInfo
                                └── AssemblyWriters = TinyDSPInstPrinter
```

## 10. 一句话解释

> `TinyDSP.td` is the top-level TableGen description file for the TinyDSP backend. It includes the target-independent LLVM TableGen definitions, pulls in TinyDSP’s register, calling convention, scheduling, and instruction descriptions, defines the generic processor model and assembly writer, and finally declares `TinyDSP` as an LLVM `Target`.

中文：

> `TinyDSP.td` 是 TinyDSP 后端的 TableGen 总入口，它引入 LLVM 通用 target 描述基础设施，再引入 TinyDSP 的寄存器、调用约定、调度模型和指令定义，最后声明 `TinyDSP` 这个 LLVM Target，并指定它使用 `TinyDSPInstrInfo` 和 `TinyDSPInstPrinter`。
