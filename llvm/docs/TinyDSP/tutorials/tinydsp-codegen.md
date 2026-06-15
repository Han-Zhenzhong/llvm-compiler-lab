
---

# TinyDSP CodeGen 总览

```text
┌─────────────────────────────────────────────────────────────┐
│                    TinyDSP CodeGen Layer                    │
│                                                             │
│      目标：把 LLVM IR / SelectionDAG 降低成 TinyDSP 机器指令  │
└─────────────────────────────────────────────────────────────┘

LLVM IR
  │
  v
SelectionDAG
  │
  │ 1. Lowering
  v
Target-specific DAG
  │
  │ 2. Instruction Selection
  v
TinyDSP MachineInstr
  │
  │ 3. Register / Frame / Machine-level handling
  v
Final MachineInstr
  │
  │ 4. AsmPrinter / MCInst lowering
  v
TinyDSP Assembly / Object
```

CodeGen 的核心不是“打印汇编”这么简单，而是完成这几件事：

```text
LLVM IR 中的通用语义
        ↓
适配 TinyDSP ABI、寄存器、指令能力
        ↓
选择 TinyDSP 机器指令
        ↓
处理寄存器、栈帧、函数调用、返回值
        ↓
输出 TinyDSP 汇编或对象文件
```

---

# 1. CodeGen 在 TinyDSP 后端中的位置


```text
llvm/lib/Target/TinyDSP/
```

其中 CodeGen 相关文件位于顶层 C++ 文件中，和 `TargetInfo/`、`MCTargetDesc/` 分层配合。GitHub 目录中可以看到 TinyDSP 后端包含 `TinyDSPAsmPrinter.cpp`、`TinyDSPFrameLowering.cpp`、`TinyDSPInstrInfo.cpp`、`TinyDSPISelDAGToDAG.cpp`、`TinyDSPISelLowering.cpp`、`TinyDSPMCInstLower.cpp`、`TinyDSPRegisterInfo.cpp`、`TinyDSPSubtarget.cpp`、`TinyDSPTargetMachine.cpp` 等 CodeGen 文件。([GitHub][2])

```text
llvm/lib/Target/TinyDSP/
│
├── TargetInfo/
│   └── TinyDSPTargetInfo.cpp
│       作用：注册 tinydsp target
│
├── MCTargetDesc/
│   └── TinyDSP MC layer
│       作用：MCInstrInfo / MCRegisterInfo / encoder / asm backend
│
└── CodeGen C++ layer
    ├── TinyDSPTargetMachine.cpp
    ├── TinyDSPSubtarget.cpp
    ├── TinyDSPISelLowering.cpp
    ├── TinyDSPISelDAGToDAG.cpp
    ├── TinyDSPInstrInfo.cpp
    ├── TinyDSPRegisterInfo.cpp
    ├── TinyDSPFrameLowering.cpp
    ├── TinyDSPAsmPrinter.cpp
    └── TinyDSPMCInstLower.cpp
```

可以理解为：

```text
TargetInfo
    让 LLVM 知道 TinyDSP 存在

TargetMachine / Subtarget
    让 LLVM 知道如何为 TinyDSP 配置 CodeGen pipeline

CodeGen
    真正把 LLVM IR / DAG / MachineInstr 变成 TinyDSP 指令

MC layer
    把 TinyDSP 指令打印成汇编或编码成 object
```

---

# 2. TinyDSP CodeGen 的主流程

```text
┌─────────────────────────────────────────────────────────────┐
│ Input: LLVM IR                                               │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPTargetMachine                                         │
│                                                             │
│ 创建 TinyDSP 的 CodeGen pipeline                            │
│ 设置 DataLayout / RelocationModel / CodeModel / OptLevel     │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPSubtarget                                             │
│                                                             │
│ 提供具体 CPU/feature 下的 backend components                 │
│ InstrInfo / RegisterInfo / FrameLowering / TargetLowering    │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPISelLowering                                          │
│                                                             │
│ 把 LLVM generic DAG 降低成 TinyDSP 可以处理的形式             │
│ 例如参数、返回值、global address、operation lowering          │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPISelDAGToDAG                                          │
│                                                             │
│ 使用手写选择逻辑 + TableGen generated matcher                │
│ 把 SelectionDAG node 选择成 TinyDSP MachineInstr             │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ Machine-level CodeGen                                        │
│                                                             │
│ TinyDSPInstrInfo                                             │
│ TinyDSPRegisterInfo                                          │
│ TinyDSPFrameLowering                                         │
│                                                             │
│ 处理指令行为、寄存器信息、栈帧、prologue/epilogue             │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPAsmPrinter + TinyDSPMCInstLower                       │
│                                                             │
│ MachineInstr → MCInst → Assembly                             │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ Output: TinyDSP assembly / object                            │
└─────────────────────────────────────────────────────────────┘
```

一句话：

```text
TinyDSP CodeGen = TargetMachine 配置流水线
                + Subtarget 提供目标信息
                + ISelLowering 适配语义
                + DAGToDAG 选择指令
                + Instr/Register/Frame 处理机器层细节
                + AsmPrinter/MCInstLower 输出汇编
```

---

# 3. CodeGen 文件职责图

```text
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPTargetMachine.cpp                                     │
├─────────────────────────────────────────────────────────────┤
│ TinyDSP 后端的 CodeGen 总入口                                │
│                                                             │
│ 负责：                                                       │
│ - 注册 TargetMachine                                         │
│ - 设置 DataLayout                                            │
│ - 创建 PassConfig                                            │
│ - 把 TinyDSP instruction selector 加入 pipeline              │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────────┐
│ TinyDSPSubtarget.cpp                                         │
├─────────────────────────────────────────────────────────────┤
│ 每个函数实际使用的 target-specific context                   │
│                                                             │
│ 负责：                                                       │
│ - CPU / feature parsing                                      │
│ - 初始化 InstrInfo / FrameLowering / TargetLowering          │
│ - 提供 getInstrInfo(), getRegisterInfo(), getFrameLowering() │
└───────────────────────┬─────────────────────────────────────┘
                        │
        ┌───────────────┼────────────────┬─────────────────┐
        │               │                │                 │
        v               v                v                 v
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────────┐
│ ISelLowering │ │ DAGToDAG ISel│ │ RegisterInfo │ │ FrameLowering  │
│              │ │              │ │              │ │                │
│ IR/DAG语义适配│ │ DAG选择成指令 │ │ 物理寄存器信息│ │ 栈帧/序言/尾声  │
└──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬─────────┘
       │                │                │                │
       └────────────────┴────────────────┴────────────────┘
                                │
                                v
                    ┌──────────────────────┐
                    │ TinyDSPInstrInfo      │
                    │ 指令行为/拷贝/分支等  │
                    └───────────┬──────────┘
                                │
                                v
                    ┌──────────────────────┐
                    │ TinyDSPAsmPrinter     │
                    │ MachineInstr → asm     │
                    └───────────┬──────────┘
                                │
                                v
                    ┌──────────────────────┐
                    │ TinyDSPMCInstLower    │
                    │ MachineInstr → MCInst  │
                    └──────────────────────┘
```

---

# 4. TargetMachine：CodeGen pipeline 的入口

`TinyDSPTargetMachine` 是 TinyDSP 后端进入 LLVM CodeGen 的总入口。

```text
┌─────────────────────────────────────────────┐
│ TinyDSPTargetMachine                         │
├─────────────────────────────────────────────┤
│ “LLVM 要为 TinyDSP 编译时，先创建我。”        │
│                                             │
│ 主要职责：                                   │
│ - 描述 TinyDSP 的 DataLayout                 │
│ - 保存 target triple                         │
│ - 创建 Subtarget                             │
│ - 创建 CodeGen pass config                   │
│ - 安装 TinyDSP instruction selector          │
└─────────────────────────────────────────────┘
```

调用关系：

```text
llc -mtriple=tinydsp input.ll
    │
    v
TargetRegistry::lookupTarget("tinydsp")
    │
    v
create TinyDSPTargetMachine
    │
    v
create TinyDSPPassConfig
    │
    v
addInstSelector()
    │
    v
createTinyDSPISelDag()
```

也就是：

```text
TargetMachine 不直接选择每条指令。
TargetMachine 负责把 TinyDSP 的 CodeGen pass 放进 LLVM pipeline。
```

---

# 5. Subtarget：每个函数的目标配置上下文

`TinyDSPSubtarget` 表示某个具体 TinyDSP CPU / feature 组合。

虽然现在 TinyDSP 可能只有一个 `generic` CPU，但 LLVM 后端通常都保留 Subtarget 抽象。

```text
┌─────────────────────────────────────────────┐
│ TinyDSPSubtarget                             │
├─────────────────────────────────────────────┤
│ “这个函数要按照哪种 TinyDSP CPU/feature 编译？”│
│                                             │
│ 提供：                                      │
│ - TinyDSPInstrInfo                           │
│ - TinyDSPRegisterInfo                        │
│ - TinyDSPFrameLowering                       │
│ - TinyDSPTargetLowering                      │
└─────────────────────────────────────────────┘
```

图示：

```text
MachineFunction
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

面试解释：

```text
TargetMachine 是 target-level 配置。
Subtarget 是 function-level 的具体 target variant 配置。
```

---

# 6. ISelLowering：把 LLVM 通用语义适配到 TinyDSP

`TinyDSPISelLowering` 是 CodeGen 里非常关键的一层。

它处理的问题是：

```text
LLVM IR / generic SelectionDAG 里的语义
        ↓
TinyDSP 的 ABI、寄存器、指令能力
```

图示：

```text
LLVM generic DAG
    │
    │ TinyDSPISelLowering
    v
TinyDSP-compatible DAG
```

它通常处理：

```text
LowerFormalArguments
    函数参数怎么进入 TinyDSP 函数？
    例如参数放 R2-R5，或从栈上取。

LowerReturn
    返回值怎么返回？
    例如返回值放 R0/R1。

LowerGlobalAddress
    global symbol/address 如何表示？

LowerOperation
    某些 LLVM generic operation 如果 TinyDSP 不能直接支持，
    需要 lowering 成 TinyDSP 可以处理的 DAG 形式。
```

图示：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPISelLowering                          │
├─────────────────────────────────────────────┤
│ Input: generic SelectionDAG                  │
│ Output: TinyDSP-compatible SelectionDAG      │
│                                             │
│ 解决：                                      │
│ - 函数参数 ABI                               │
│ - 返回值 ABI                                 │
│ - global address                             │
│ - 不合法 operation 的 lowering               │
└─────────────────────────────────────────────┘
```

例子：

```text
C code:
    int add(int a, int b) { return a + b; }

LLVM semantic:
    argument a
    argument b
    add
    return

TinyDSP lowering 后的语义:
    a from R2
    b from R3
    ADD into R0
    RET
```

---

# 7. DAGToDAG：真正选择 TinyDSP 指令

`TinyDSPISelDAGToDAG.cpp` 的核心职责是：

```text
SelectionDAG node → TinyDSP MachineInstr
```

图示：

```text
SelectionDAG
    │
    │ TinyDSPISelDAGToDAG
    v
MachineInstr
```

更细：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPISelDAGToDAG.cpp                      │
├─────────────────────────────────────────────┤
│ 手写选择逻辑                                 │
│ +                                           │
│ TableGen generated matcher                  │
│                                             │
│ 解决：                                      │
│ - add node 选择为 ADD 指令                   │
│ - sub node 选择为 SUB 指令                   │
│ - mul node 选择为 MUL 指令                   │
│ - load/store node 选择为 LOAD/STORE          │
└─────────────────────────────────────────────┘
```

抽象例子：

```text
DAG node:
    add i32:$lhs, i32:$rhs

TableGen pattern / selector:
    (add GPR:$lhs, GPR:$rhs) -> ADD GPR:$lhs, GPR:$rhs

MachineInstr:
    ADD R0, R2, R3
```

可以理解成：

```text
ISelLowering 负责“让 DAG 合法化 / 适配 TinyDSP”
DAGToDAG 负责“把 DAG 变成 TinyDSP 指令”
```

---

# 8. InstrInfo：机器指令行为信息

`TinyDSPInstrInfo` 描述 TinyDSP 机器指令在 LLVM CodeGen 中的行为。

它和 `.td` 文件共同工作：

```text
TinyDSPInstrInfo.td
    声明指令格式、操作数、pattern、asm string

TinyDSPInstrInfo.cpp
    实现 LLVM CodeGen 需要的指令级 hook
```

图示：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPInstrInfo                             │
├─────────────────────────────────────────────┤
│ 代表 TinyDSP 指令层面的 target-specific 知识 │
│                                             │
│ 可能处理：                                  │
│ - copyPhysReg                                │
│ - storeRegToStackSlot                        │
│ - loadRegFromStackSlot                       │
│ - 分支分析 / 分支插入 / 分支删除             │
│ - 指令属性查询                               │
└─────────────────────────────────────────────┘
```

在 LLVM 后端里，很多机器级 pass 会问 `InstrInfo`：

```text
“如何复制一个物理寄存器？”
“如何把寄存器 spill 到 stack slot？”
“如何从 stack slot reload？”
“这个 MachineInstr 是不是 branch？”
“这个指令会不会修改某个寄存器？”
```

所以它不是“指令列表”而已，而是：

```text
TinyDSP 指令如何被 LLVM machine-level passes 使用
```

---

# 9. RegisterInfo：寄存器信息

`TinyDSPRegisterInfo` 描述 TinyDSP 的物理寄存器和寄存器相关规则。

结合你的 high-level 文档，TinyDSP ISA 里有 `R0-R7`，其中 `R0-R1` 用于返回值，`R2-R5` 用于参数，`R6` 作为 FP，`R7` 作为 SP。([GitHub][1])

```text
┌─────────────────────────────────────────────┐
│ TinyDSPRegisterInfo                          │
├─────────────────────────────────────────────┤
│ 负责：                                      │
│ - 有哪些物理寄存器                           │
│ - 哪些寄存器是 reserved                      │
│ - frame register 是谁                        │
│ - callee-saved registers                     │
│ - register class 信息                        │
│ - frame index 如何消除                       │
└─────────────────────────────────────────────┘
```

图示：

```text
TinyDSP physical registers
    │
    ├── R0 / R1
    │     └── return value
    │
    ├── R2 / R3 / R4 / R5
    │     └── argument registers
    │
    ├── R6
    │     └── frame pointer
    │
    └── R7
          └── stack pointer
```

LLVM machine pass 会通过 RegisterInfo 理解：

```text
哪些寄存器可分配？
哪些寄存器不能分配？
栈帧访问时 frame index 最终要替换成哪个 base register + offset？
```

---

# 10. FrameLowering：栈帧、prologue、epilogue

`TinyDSPFrameLowering` 负责函数栈帧。

```text
┌─────────────────────────────────────────────┐
│ TinyDSPFrameLowering                         │
├─────────────────────────────────────────────┤
│ 负责：                                      │
│ - 函数入口 prologue                          │
│ - 函数退出 epilogue                          │
│ - stack pointer 调整                         │
│ - frame pointer 使用策略                     │
│ - local variable / spill slot 的栈空间布局    │
└─────────────────────────────────────────────┘
```

图示：

```text
function entry
    │
    v
Prologue
    │
    ├── adjust SP
    ├── save callee-saved registers
    └── setup FP if needed

function body
    │
    ├── local variables
    ├── spills
    └── stack slots

function exit
    │
    v
Epilogue
    │
    ├── restore callee-saved registers
    ├── restore SP
    └── RET
```

简化后的 TinyDSP 栈帧模型：

```text
higher address
┌────────────────────────┐
│ caller stack area       │
├────────────────────────┤
│ return address / saved  │
├────────────────────────┤
│ callee-saved regs       │
├────────────────────────┤
│ local variables         │
├────────────────────────┤
│ spill slots             │
└────────────────────────┘
lower address

R7 = SP
R6 = FP
```

面试时可以说：

```text
FrameLowering connects LLVM's abstract stack frame model with TinyDSP's concrete SP/FP convention.
```

---

# 11. AsmPrinter / MCInstLower：从 MachineInstr 到汇编

到这一步，LLVM 已经有 TinyDSP MachineInstr 了，但还不是最终文本汇编或对象文件。

```text
MachineInstr
    │
    │ TinyDSPAsmPrinter
    v
MCInst
    │
    │ TinyDSPMCInstLower
    v
MC layer
    │
    v
assembly / object
```

图示：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPAsmPrinter                            │
├─────────────────────────────────────────────┤
│ 负责：                                      │
│ - 遍历 MachineFunction                       │
│ - 输出函数 label                             │
│ - 输出每条 MachineInstr                      │
│ - 调用 MCInstLower 转换到 MCInst             │
└──────────────────────┬──────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────┐
│ TinyDSPMCInstLower                           │
├─────────────────────────────────────────────┤
│ 负责：                                      │
│ - MachineOperand → MCOperand                 │
│ - MachineInstr → MCInst                      │
│ - symbol / immediate / register operand 处理 │
└──────────────────────┬──────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────┐
│ MCTargetDesc                                 │
├─────────────────────────────────────────────┤
│ - TinyDSPInstPrinter                         │
│ - TinyDSPMCCodeEmitter                       │
│ - TinyDSPAsmBackend                          │
│ - TinyDSPELFObjectWriter                     │
└─────────────────────────────────────────────┘
```

可以这样理解：

```text
CodeGen 层关注 MachineInstr。
MC 层关注 MCInst / encoding / object。
AsmPrinter 和 MCInstLower 是 CodeGen 到 MC 的桥。
```

---

# 12. 从 C 函数到 TinyDSP 指令的完整例子

假设 C 代码是：

```c
int add(int a, int b) {
    return a + b;
}
```

整体路径：

```text
C source
  │
  │ clang -target tinydsp
  v
LLVM IR
  │
  │ function args: %a, %b
  │ return: add %a, %b
  v
SelectionDAG
  │
  │ generic add node
  v
TinyDSPISelLowering
  │
  │ map arguments to TinyDSP calling convention
  │ a -> R2
  │ b -> R3
  │ return value -> R0
  v
TinyDSPISelDAGToDAG
  │
  │ select add node into TinyDSP ADD instruction
  v
MachineInstr
  │
  │ ADD R0, R2, R3
  │ RET
  v
AsmPrinter / MC layer
  │
  v
TinyDSP assembly
```

图示：

```text
┌──────────────┐
│ int add(a,b) │
└──────┬───────┘
       v
┌──────────────────────────────┐
│ LLVM IR                       │
│ %0 = add i32 %a, %b           │
│ ret i32 %0                    │
└──────┬───────────────────────┘
       v
┌──────────────────────────────┐
│ SelectionDAG                  │
│ add node + return node        │
└──────┬───────────────────────┘
       v
┌──────────────────────────────┐
│ TinyDSP lowering              │
│ a -> R2                       │
│ b -> R3                       │
│ retval -> R0                  │
└──────┬───────────────────────┘
       v
┌──────────────────────────────┐
│ TinyDSP MachineInstr          │
│ ADD R0, R2, R3                │
│ RET                           │
└──────┬───────────────────────┘
       v
┌──────────────────────────────┐
│ TinyDSP assembly              │
└──────────────────────────────┘
```

---

# 13. CodeGen 和 TableGen 的关系

CodeGen C++ 文件不是孤立工作的，它大量依赖 TableGen 生成的 `.inc` 文件。

```text
TinyDSP .td files
    │
    │ llvm-tblgen
    v
TinyDSPGen*.inc
    │
    v
TinyDSP C++ CodeGen files
```

对应关系：

```text
TinyDSPRegisterInfo.td
    ↓
TinyDSPGenRegisterInfo.inc
    ↓
TinyDSPRegisterInfo.cpp

TinyDSPInstrInfo.td
    ↓
TinyDSPGenInstrInfo.inc
    ↓
TinyDSPInstrInfo.cpp

TinyDSPCallingConv.td
    ↓
TinyDSPGenCallingConv.inc
    ↓
TinyDSPISelLowering.cpp

TinyDSPInstrInfo.td patterns
    ↓
TinyDSPGenDAGISel.inc
    ↓
TinyDSPISelDAGToDAG.cpp

TinyDSPSchedule.td
    ↓
TinyDSPGenSubtargetInfo.inc
    ↓
TinyDSPSubtarget.cpp

AsmWriter definition
    ↓
TinyDSPGenAsmWriter.inc
    ↓
Asm printing path
```

所以 CodeGen 的一半是：

```text
TableGen declarative description
```

另一半是：

```text
C++ target-specific hooks
```

图示：

```text
┌──────────────────────────────┐
│ Declarative side              │
│ .td files                     │
├──────────────────────────────┤
│ registers                     │
│ instruction formats           │
│ instruction definitions        │
│ calling convention             │
│ scheduling model               │
│ DAG patterns                   │
└──────────────┬───────────────┘
               │ tablegen
               v
┌──────────────────────────────┐
│ Generated side                │
│ TinyDSPGen*.inc               │
└──────────────┬───────────────┘
               │ included by
               v
┌──────────────────────────────┐
│ C++ CodeGen side              │
│ TinyDSP*.cpp                  │
├──────────────────────────────┤
│ lowering hooks                │
│ frame hooks                   │
│ register hooks                │
│ instruction selector           │
│ asm printer bridge             │
└──────────────────────────────┘
```

---

# 14. CodeGen 和 MC layer 的边界

容易混淆的一点是：

```text
CodeGen 不是 MC。
MC 也不是 CodeGen。
```

边界如下：

```text
┌─────────────────────────────────────────────┐
│ CodeGen                                      │
├─────────────────────────────────────────────┤
│ LLVM IR / DAG / MachineInstr                 │
│                                             │
│ 关注：                                      │
│ - 如何 lowering                              │
│ - 如何选择指令                               │
│ - 如何分配寄存器                             │
│ - 如何处理栈帧                               │
│ - 如何生成 MachineInstr                      │
└──────────────────────┬──────────────────────┘
                       │
                       │ MachineInstr → MCInst
                       v
┌─────────────────────────────────────────────┐
│ MC Layer                                     │
├─────────────────────────────────────────────┤
│ MCInst / MCOperand / encoding / object       │
│                                             │
│ 关注：                                      │
│ - 汇编打印                                   │
│ - 指令编码                                   │
│ - relocation/fixup                           │
│ - ELF object writer                          │
└─────────────────────────────────────────────┘
```

更短的说法：

```text
CodeGen 负责“选什么指令”。
MC 负责“怎么打印/编码这条指令”。
```

---

# 15. 总结


> TinyDSP CodeGen is the target-specific part of LLVM that lowers LLVM IR into TinyDSP machine instructions. The `TinyDSPTargetMachine` creates the code generation pipeline, while `TinyDSPSubtarget` provides target-specific components such as instruction info, register info, frame lowering, and target lowering. `TinyDSPISelLowering` adapts generic LLVM DAG operations to TinyDSP ABI and instruction constraints, and `TinyDSPISelDAGToDAG` selects TinyDSP machine instructions using TableGen-generated patterns and custom selection logic. After machine-level processing, `TinyDSPAsmPrinter` and `TinyDSPMCInstLower` bridge MachineInstr to the MC layer for assembly or object emission.

中文版本：

> TinyDSP CodeGen 是 LLVM 后端中把 LLVM IR 降低成 TinyDSP 机器指令的 target-specific 部分。`TinyDSPTargetMachine` 负责创建 CodeGen pipeline，`TinyDSPSubtarget` 提供指令、寄存器、栈帧和 lowering 等目标相关组件。`TinyDSPISelLowering` 负责把 LLVM 通用 DAG 语义适配到 TinyDSP ABI 和指令能力，`TinyDSPISelDAGToDAG` 负责把 DAG node 选择成 TinyDSP MachineInstr。之后经过寄存器、栈帧等 machine-level 处理，再由 `TinyDSPAsmPrinter` 和 `TinyDSPMCInstLower` 进入 MC 层输出汇编或对象文件。

最核心的一张图是：

```text
LLVM IR
  ↓
SelectionDAG
  ↓  TinyDSPISelLowering
TinyDSP-compatible DAG
  ↓  TinyDSPISelDAGToDAG
TinyDSP MachineInstr
  ↓  RegisterInfo / InstrInfo / FrameLowering
Final MachineInstr
  ↓  AsmPrinter / MCInstLower
MCInst
  ↓  MC layer
TinyDSP assembly / object
```

这就是 TinyDSP CodeGen 的 high-level 内容。
