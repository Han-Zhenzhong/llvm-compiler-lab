# TinyDSP ISA Design

```text
┌─────────────────────────────────────────────────────────────┐
│                 TinyDSP LLVM Backend Project                │
│        Educational + Practical LLVM backend experiment       │
└─────────────────────────────────────────────────────────────┘

                         ┌────────────────────┐
                         │  TinyDSP ISA Design │
                         └─────────┬──────────┘
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
        v                          v                          v
┌───────────────┐          ┌────────────────┐          ┌────────────────┐
│ Registers     │          │ Instructions   │          │ Encoding / ISA  │
│ R0-R7          │          │ ADD/SUB/MUL    │          │ 32-bit fixed    │
│ R0-R1 return   │          │ ADDI           │          │ R-Type/I-Type   │
│ R2-R5 args     │          │ AND/OR/XOR     │          │ M-Type          │
│ R6 FP          │          │ LOAD/STORE     │          │ little-endian   │
│ R7 SP          │          │ RET            │          │                │
└───────────────┘          └────────────────┘          └────────────────┘
```

# TinyDSP implementations under llvm/lib/Target/TinyDSP

```text
┌─────────────────────────────────────────────────────────────┐
│                llvm/lib/Target/TinyDSP                      │
└─────────────────────────────────────────────────────────────┘

  1. Target 注册层
  ┌──────────────────────────────────────┐
  │ TargetInfo/TinyDSPTargetInfo.cpp     │
  │                                      │
  │ 作用：让 LLVM 知道存在 tinydsp target │
  └──────────────────────────────────────┘

  2. TargetMachine / Subtarget 层
  ┌──────────────────────────────────────┐
  │ TinyDSPTargetMachine.{h,cpp}         │
  │ TinyDSPSubtarget.{h,cpp}             │
  │                                      │
  │ 作用：描述目标机器、子架构、DataLayout │
  └──────────────────────────────────────┘

  3. TableGen 描述层
  ┌──────────────────────────────────────┐
  │ TinyDSP.td                           │
  │ TinyDSPRegisterInfo.td               │
  │ TinyDSPInstrInfo.td                  │
  │ TinyDSPInstrFormats.td               │
  │ TinyDSPCallingConv.td                │
  │ TinyDSPSchedule.td                   │
  │                                      │
  │ 作用：寄存器、指令、格式、调用约定、调度模型 │
  └──────────────────────────────────────┘

  4. CodeGen C++ 实现层
  ┌──────────────────────────────────────┐
  │ TinyDSPISelLowering.{h,cpp}          │
  │ TinyDSPISelDAGToDAG.cpp              │
  │ TinyDSPInstrInfo.{h,cpp}             │
  │ TinyDSPRegisterInfo.{h,cpp}          │
  │ TinyDSPFrameLowering.{h,cpp}         │
  │ TinyDSPAsmPrinter.cpp                │
  │ TinyDSPMCInstLower.{h,cpp}           │
  │                                      │
  │ 作用：IR/SelectionDAG → MachineInstr → asm │
  └──────────────────────────────────────┘

  5. MC 层
  ┌──────────────────────────────────────┐
  │ MCTargetDesc/                        │
  │   TinyDSPMCAsmInfo                   │
  │   TinyDSPInstPrinter                 │
  │   TinyDSPMCCodeEmitter               │
  │   TinyDSPAsmBackend                  │
  │   TinyDSPELFObjectWriter             │
  │                                      │
  │ 作用：汇编打印、机器码编码、ELF 对象文件生成 │
  └──────────────────────────────────────┘
```

# LLVM processing flow for TinyDSP C source code

```text
C source
  │
  │ clang -target tinydsp
  v
LLVM IR
  │
  │ LLVM middle-end optimization
  v
SelectionDAG
  │
  │ TinyDSPISelLowering
  │ TinyDSPISelDAGToDAG
  v
MachineInstr
  │
  │ Register allocation
  │ Frame lowering
  │ Instruction scheduling
  v
TinyDSP assembly
  │
  │ MC layer
  │ AsmPrinter / MCCodeEmitter / AsmBackend
  v
TinyDSP object file / ELF
```

# LLVM IR -> TinyDSP Instruction Selection -> TinyDSP ASM

```text
LLVM IR
  │
  v
┌──────────────────────────────┐
│ TinyDSPISelLowering.cpp      │
│                              │
│ - LowerFormalArguments       │
│ - LowerReturn                │
│ - LowerGlobalAddress         │
│ - LowerOperation             │
│                              │
│ 解决：LLVM IR/SDNode 如何适配 TinyDSP ABI 和指令能力 │
└───────────────┬──────────────┘
                │
                v
┌──────────────────────────────┐
│ TinyDSPISelDAGToDAG.cpp      │
│                              │
│ - SelectionDAG pattern match │
│ - TableGen generated matcher │
│                              │
│ 解决：DAG node 选择成 TinyDSP MachineInstr │
└───────────────┬──────────────┘
                │
                v
┌──────────────────────────────┐
│ TinyDSPInstrInfo.td/.cpp     │
│ TinyDSPRegisterInfo.td/.cpp  │
│ TinyDSPCallingConv.td        │
│ TinyDSPFrameLowering.cpp     │
│                              │
│ 解决：指令、寄存器、调用约定、栈帧、prologue/epilogue │
└───────────────┬──────────────┘
                │
                v
┌──────────────────────────────┐
│ TinyDSPAsmPrinter.cpp        │
│ TinyDSPMCInstLower.cpp       │
│ MCTargetDesc/*               │
│                              │
│ 解决：MachineInstr → MCInst → asm/object │
└───────────────┬──────────────┘
                │
                v
        TinyDSP .s / .o
```

# Verification flow

```text
┌──────────────────────────────┐
│ llvm/test/CodeGen/TinyDSP    │
│ Inputs/test_basic.c          │
└───────────────┬──────────────┘
                │
                │ verify_tinydsp.py
                v
┌──────────────────────────────┐
│ clang -target tinydsp -S     │
│ C code → TinyDSP assembly    │
└───────────────┬──────────────┘
                │
                v
┌──────────────────────────────┐
│ Extract function assembly    │
│ test_add / test_sub / etc.   │
└───────────────┬──────────────┘
                │
                v
┌──────────────────────────────┐
│ tinydsp_simulator.py         │
│                              │
│ - set args into R2-R5        │
│ - execute assembly           │
│ - read return value from R0  │
│ - compare expected result    │
└───────────────┬──────────────┘
                │
                v
        PASS / FAIL report
```

# Values of this experiment

```text
┌──────────────────────┐
│ Educational Value     │
├──────────────────────┤
│ LLVM backend skeleton │
│ TableGen usage        │
│ instruction selection │
│ register allocation   │
│ calling convention    │
│ frame lowering        │
└──────────┬───────────┘
           │
           v
┌──────────────────────┐
│ Engineering Value     │
├──────────────────────┤
│ build integration     │
│ clang target support  │
│ llc lowering          │
│ asm/object generation │
│ automated verification│
└──────────┬───────────┘
           │
           v
┌──────────────────────┐
│ Future Real Value     │
├──────────────────────┤
│ custom ISA prototype  │
│ DSP instruction design│
│ simulator co-design   │
│ RTL/FPGA integration  │
│ performance modeling  │
└──────────────────────┘
```

# Overall done stuff in this experiment

```text
自定义 ISA
    ↓
LLVM 后端描述
    ↓
C/LLVM IR 降低到目标汇编
    ↓
模拟器验证语义正确性
    ↓
具备 compiler + ISA + simulator co-design 的完整闭环
```
