
```text
TinyDSP 自己主要实现的是 target-specific backend hooks。

IR 优化、寄存器分配、很多机器级 pass、部分指令调度框架，
主要用的是 LLVM existing infrastructure / existing algorithms。
```

但要分层看。

---

## 1. TinyDSP 自己有没有实现 IR 优化？

**基本没有。**

TinyDSP 后端目前没有新增类似：

```text
TinyDSP-specific IR optimization pass
TinyDSP-specific Loop pass
TinyDSP-specific Function pass
TinyDSP-specific GVN/SCCP/InstCombine pass
```

这种 IR 层优化。

`high-level-view.md` 里写的是：

```text
C source
  ↓ clang -target tinydsp
LLVM IR
  ↓ LLVM middle-end optimization
SelectionDAG
  ↓ TinyDSPISelLowering / TinyDSPISelDAGToDAG
MachineInstr
```

也就是说，**IR 优化来自 LLVM/Clang 通用 middle-end pipeline**，不是 TinyDSP 自己写的优化。TinyDSP 后端是从 LLVM IR 后面的 CodeGen 阶段开始发挥作用。

图示：

```text
C source
  │
  │ clang -O1/-O2/-O3
  v
LLVM IR
  │
  │ LLVM existing IR optimizations
  │ InstCombine / SimplifyCFG / GVN / SROA / DCE / Loop opts ...
  v
Optimized LLVM IR
  │
  │ enter TinyDSP backend
  v
SelectionDAG / MachineInstr / asm
```

准确的说：

> TinyDSP currently does not implement custom IR optimization passes. It reuses LLVM’s existing middle-end optimization pipeline. TinyDSP-specific work starts mainly from instruction lowering and instruction selection.

---

## 2. 指令选择是不是用了 LLVM existing algorithm？

**是，用了 LLVM 的 SelectionDAG instruction selection 框架；但 TinyDSP 提供了 target-specific lowering、pattern 和少量 custom selection。**

`TinyDSPTargetMachine.cpp` 里 `TinyDSPPassConfig::addInstSelector()` 把 TinyDSP 的 instruction selector 加进 LLVM CodeGen pipeline：

```cpp
bool TinyDSPPassConfig::addInstSelector() {
  addPass(createTinyDSPISelDag(getTinyDSPTargetMachine(), getOptLevel()));
  return false;
}
```

这说明 LLVM 通用 CodeGen pipeline 会在合适阶段调用 TinyDSP 的 DAG-to-DAG instruction selector。

图示：

```text
LLVM CodeGen pipeline
  │
  ├── IR legalization
  ├── SelectionDAG construction
  ├── DAG legalization
  │
  ├── addInstSelector()
  │       ↓
  │   createTinyDSPISelDag()
  │       ↓
  │   TinyDSPISelDAGToDAG
  │
  ├── Register allocation
  ├── Machine scheduling
  └── AsmPrinter
```

`TinyDSPISelDAGToDAG.cpp` 继承了 LLVM 的 `SelectionDAGISel`，并 include 了 TableGen 生成的 `TinyDSPGenDAGISel.inc`。同时写了 `selectAddr`、`trySelectLoad`、`trySelectStore`、`trySelectShiftLeft` 等逻辑，最后 fallback 到 `SelectCode(Node)`，也就是 TableGen generated matcher。

图示：

```text
SelectionDAG node
  │
  v
TinyDSPDAGToDAGISel::Select()
  │
  ├── custom selection
  │     ├── LOAD
  │     ├── STORE
  │     └── SHL by 1 → ADD src, src
  │
  └── SelectCode(Node)
        ↓
      TableGen generated matcher
        ↓
      TinyDSPGenDAGISel.inc
```

所以更准确的表达是：

```text
指令选择算法框架 = LLVM existing SelectionDAGISel
TinyDSP 提供内容 = lowering hooks + TableGen patterns + custom Select logic
```

---

## 3. 寄存器分配是不是用了 LLVM existing algorithm？

**是。TinyDSP 没有自己实现 register allocator。它使用 LLVM 通用寄存器分配器。**

通常 LLVM 后端会复用 LLVM 的机器级寄存器分配框架，例如 greedy register allocator、fast register allocator 等，具体取决于优化级别和 pipeline 配置。

TinyDSP 需要提供的是：

```text
有哪些物理寄存器？
哪些寄存器可分配？
哪些寄存器保留？
哪些寄存器是 callee-saved？
如何 spill/reload？
如何消除 frame index？
```

这些信息通过 target hook 提供给 LLVM 的寄存器分配器。

关系图：

```text
LLVM generic register allocator
  │
  │ asks target-specific questions
  v
TinyDSPRegisterInfo / TinyDSPInstrInfo
  │
  ├── getReservedRegs()
  ├── getCalleeSavedRegs()
  ├── getFrameRegister()
  ├── eliminateFrameIndex()
  ├── storeRegToStackSlot()
  ├── loadRegFromStackSlot()
  └── copyPhysReg()
```

`TinyDSPRegisterInfo.cpp` 里保留了 `R7` 作为 stack pointer，`R6` 作为 frame pointer，并实现了 `eliminateFrameIndex()`，把 LLVM 抽象的 frame index 替换成 `R7 + offset` 形式。

`TinyDSPInstrInfo.cpp` 里实现了：

```text
copyPhysReg()
storeRegToStackSlot()
loadRegFromStackSlot()
```

其中 `copyPhysReg()` 用 `ADD dest, src, R0` 模拟 MOV；spill/reload 则用 `STORE` / `LOAD`。这些 hook 会被 LLVM 通用寄存器分配和 spill/reload 逻辑调用。

图示：

```text
Virtual registers
  │
  │ LLVM register allocator
  v
Physical registers
  │
  ├── assign vreg → R0/R1/R2/...
  ├── avoid reserved regs R6/R7
  ├── spill if not enough regs
  │       ↓
  │   TinyDSPInstrInfo::storeRegToStackSlot()
  │       ↓
  │   STORE
  │
  ├── reload spilled value
  │       ↓
  │   TinyDSPInstrInfo::loadRegFromStackSlot()
  │       ↓
  │   LOAD
  │
  └── eliminate frame index
          ↓
      TinyDSPRegisterInfo::eliminateFrameIndex()
          ↓
      [FI] → R7 + offset
```

One sentance:

> I did not implement a new register allocation algorithm. TinyDSP reuses LLVM’s generic register allocation infrastructure. My backend provides the register classes, reserved registers, callee-saved information, spill/reload hooks, copyPhysReg, and frame index elimination, so LLVM’s existing register allocator can work on TinyDSP MachineInstr.

---

## 4. 指令调度是不是用了 LLVM existing algorithm？

**大体是。TinyDSP 没有自己写一个 instruction scheduler。它依赖 LLVM 的通用 machine scheduler / post-RA scheduler 框架。**

但 LLVM 的调度器需要 target 提供一些信息，例如：

```text
指令 latency
processor model
resource model
itinerary / scheduling model
```

`TinyDSPSchedule.td` 放在 TableGen 描述层，作用是“调度模型”。

关系图：

```text
LLVM Machine Scheduler
  │
  │ reads target scheduling info
  v
TinyDSPSchedule.td
  │
  │ tablegen
  v
TinyDSPGenSubtargetInfo.inc
  │
  v
TinyDSPSubtarget
  │
  v
MachineScheduler decides instruction order
```

更完整：

```text
MachineInstr before scheduling
  │
  │ LLVM existing machine scheduler
  │
  ├── asks InstrInfo:
  │       instruction properties
  │
  ├── asks Subtarget:
  │       scheduling model
  │
  ├── asks generated itinerary/model:
  │       latency / resources
  │
  v
MachineInstr after scheduling
```

不过对当前 TinyDSP 来说，调度模型应该还是比较简单。也就是说：

```text
框架上接入了 LLVM scheduling infrastructure；
但 TinyDSP 自己的 scheduling model 目前可能只是 minimal/simple model。
```

---

## 5. 这些 LLVM existing algorithms 如何作用到 TinyDSP CodeGen？

核心机制是：

```text
LLVM 通用算法不直接认识 TinyDSP。
它们通过 TargetMachine / Subtarget / TargetInstrInfo / TargetRegisterInfo / TargetFrameLowering / TargetLowering 这些接口认识 TinyDSP。
```

总图：

```text
┌─────────────────────────────────────────────────────────────┐
│                LLVM existing CodeGen algorithms              │
│                                                             │
│ - SelectionDAG legalization                                  │
│ - SelectionDAG instruction selection framework               │
│ - register allocation                                        │
│ - spill / reload                                             │
│ - frame index elimination                                    │
│ - machine instruction scheduling                             │
│ - branch relaxation / machine-level passes                   │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            │ call target hooks / query target info
                            v
┌─────────────────────────────────────────────────────────────┐
│                    TinyDSP target-specific hooks             │
│                                                             │
│ TinyDSPTargetMachine                                         │
│ TinyDSPSubtarget                                             │
│ TinyDSPISelLowering                                          │
│ TinyDSPISelDAGToDAG                                          │
│ TinyDSPInstrInfo                                             │
│ TinyDSPRegisterInfo                                          │
│ TinyDSPFrameLowering                                         │
│ TinyDSPSchedule.td                                           │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            v
┌─────────────────────────────────────────────────────────────┐
│                    TinyDSP MachineInstr / asm                │
└─────────────────────────────────────────────────────────────┘
```

换句话说：

```text
LLVM algorithm = 通用发动机
TinyDSP backend = target-specific 插头、规则、描述文件
```

LLVM 通用算法会问 TinyDSP：

```text
这个 target 有哪些寄存器？
这个虚拟寄存器可以放进哪个 register class？
哪些物理寄存器不能用？
寄存器不够时怎么 spill？
frame index 怎么变成真实地址？
这个 DAG node 能匹配成哪条目标指令？
这条指令的 asm string 是什么？
这条指令 latency 是多少？
```

TinyDSP 通过这些文件回答：

```text
TinyDSPRegisterInfo.td/.cpp
TinyDSPInstrInfo.td/.cpp
TinyDSPCallingConv.td
TinyDSPISelLowering.cpp
TinyDSPISelDAGToDAG.cpp
TinyDSPFrameLowering.cpp
TinyDSPSchedule.td
TinyDSPAsmPrinter.cpp
MCTargetDesc/*
```

---

## 6. 一个完整例子：`int add(int a, int b)`

```c
int add(int a, int b) {
    return a + b;
}
```

编译过程：

```text
C source
  │
  │ clang -O1 -target tinydsp
  v
LLVM IR
  │
  │ LLVM existing IR optimization
  │ 例如 simplify、inline、DCE 等，取决于优化级别
  v
Optimized LLVM IR
  │
  │ LLVM SelectionDAG framework
  v
SelectionDAG
  │
  │ TinyDSPISelLowering
  │ - 参数 a/b 按 TinyDSP calling convention 放入 R2/R3
  │ - 返回值放 R0
  v
TinyDSP-compatible DAG
  │
  │ TinyDSPISelDAGToDAG + TableGen matcher
  v
TinyDSP MachineInstr
  │
  │ LLVM register allocator
  │ - 分配物理寄存器
  │ - 必要时调用 TinyDSP spill/reload hooks
  v
Allocated MachineInstr
  │
  │ LLVM machine scheduler
  │ - 根据 TinyDSP scheduling model 调整顺序
  v
Scheduled MachineInstr
  │
  │ TinyDSPAsmPrinter / MC layer
  v
TinyDSP assembly
```

可以画成更短的版本：

```text
LLVM IR
  ↓  LLVM existing middle-end opts
Optimized IR
  ↓  LLVM SelectionDAG framework + TinyDSP lowering/hooks
DAG
  ↓  LLVM SelectionDAGISel + TinyDSP patterns/custom Select
MachineInstr
  ↓  LLVM register allocator + TinyDSP RegisterInfo/InstrInfo
Allocated MachineInstr
  ↓  LLVM scheduler + TinyDSP schedule model
Scheduled MachineInstr
  ↓  TinyDSP AsmPrinter/MC
Assembly/Object
```

---

## 7. 实现的部分 vs LLVM 复用的部分

```text
┌──────────────────────────────┬──────────────────────────────┐
│ 阶段                         │ TinyDSP 是否自己实现算法？      │
├──────────────────────────────┼──────────────────────────────┤
│ IR optimization               │ 否，复用 LLVM middle-end        │
│ Instruction lowering          │ 是，TinyDSPISelLowering         │
│ Instruction selection framework│ 复用 LLVM SelectionDAGISel      │
│ Instruction selection patterns │ 是，TableGen + custom Select    │
│ Register allocation algorithm │ 否，复用 LLVM regalloc          │
│ Spill/reload target behavior  │ 是，TinyDSPInstrInfo hooks      │
│ Frame index elimination       │ 是，TinyDSPRegisterInfo hook    │
│ Instruction scheduling algo   │ 否，复用 LLVM machine scheduler │
│ Scheduling model              │ 是，TinyDSPSchedule.td          │
│ Assembly/object emission      │ 部分复用 MC，部分 TinyDSP 实现   │
└──────────────────────────────┴──────────────────────────────┘
```

---

## 8. 最准确的说法

英文版：

> TinyDSP does not implement custom LLVM IR optimization passes. It reuses LLVM’s existing middle-end optimizations. For CodeGen, it reuses LLVM’s SelectionDAG framework, generic register allocation, and machine scheduling infrastructure. The TinyDSP backend provides the target-specific pieces: TargetMachine and Subtarget setup, TableGen instruction/register descriptions, calling convention, SelectionDAG lowering, DAG-to-DAG selection hooks, register information, spill/reload hooks, frame lowering, and MC/AsmPrinter support. These target hooks allow LLVM’s generic algorithms to operate on TinyDSP MachineInstr.

中文版：

> TinyDSP 目前没有实现自定义 IR 优化 pass，而是复用 LLVM 现有 middle-end 优化。CodeGen 阶段也不是自己重写寄存器分配或指令调度算法，而是复用 LLVM 的 SelectionDAG、通用寄存器分配器和 machine scheduler。TinyDSP 自己提供的是 target-specific 信息和 hook，包括 TargetMachine/Subtarget、TableGen 指令和寄存器描述、调用约定、ISelLowering、DAGToDAG selector、RegisterInfo、InstrInfo、FrameLowering、AsmPrinter 和 MC 层。LLVM 的通用算法通过这些接口理解 TinyDSP，从而完成指令选择、寄存器分配、spill/reload、栈帧处理和指令调度。
