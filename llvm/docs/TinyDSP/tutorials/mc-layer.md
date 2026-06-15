
# 1. MC 层在 TinyDSP 后端中的位置

```text
┌─────────────────────────────────────────────────────────────┐
│                    TinyDSP LLVM Backend                     │
└─────────────────────────────────────────────────────────────┘

LLVM IR
  │
  v
SelectionDAG
  │
  v
MachineInstr
  │
  │ CodeGen layer
  │ TinyDSPISelLowering / DAGToDAG / RegAlloc / FrameLowering
  v
Final MachineInstr
  │
  │ TinyDSPAsmPrinter
  │ TinyDSPMCInstLower
  v
MCInst
  │
  │ MC layer
  v
Assembly text / Object file / ELF
```

一句话：

```text
CodeGen 层负责生成 TinyDSP MachineInstr。
MC 层负责把 MCInst 打印成汇编，或者编码成机器码/对象文件。
```

更短地说：

```text
CodeGen: 选什么指令
MC:      这条指令怎么打印、怎么编码、怎么写进 object
```

---

# 2. TinyDSP MC 层目录

TinyDSP MC 层主要在：

```text
llvm/lib/Target/TinyDSP/MCTargetDesc/
```

`MCTargetDesc/CMakeLists.txt` 把这些文件编译成 `LLVMTinyDSPDesc` 组件库：`TinyDSPAsmBackend.cpp`、`TinyDSPELFObjectWriter.cpp`、`TinyDSPInstPrinter.cpp`、`TinyDSPMCAsmInfo.cpp`、`TinyDSPMCCodeEmitter.cpp`、`TinyDSPMCTargetDesc.cpp`，并链接 `MC`、`Support`、`TinyDSPInfo`。

```text
llvm/lib/Target/TinyDSP/MCTargetDesc/
│
├── CMakeLists.txt
│   └── build LLVMTinyDSPDesc
│
├── TinyDSPMCTargetDesc.h
│   └── MC 层公共声明、寄存器/指令枚举 include
│
├── TinyDSPMCTargetDesc.cpp
│   └── 注册 MCInstrInfo / MCRegInfo / MCSubtargetInfo / CodeEmitter / AsmBackend / InstPrinter
│
├── TinyDSPMCAsmInfo.cpp
│   └── 汇编语法属性：大小端、注释符、data directive、debug info
│
├── TinyDSPInstPrinter.cpp
│   └── MCInst → TinyDSP assembly text
│
├── TinyDSPMCCodeEmitter.cpp
│   └── MCInst → binary instruction encoding
│
├── TinyDSPAsmBackend.cpp
│   └── fixup / nop / object writer backend
│
└── TinyDSPELFObjectWriter.cpp
    └── ELF object writer / relocation type
```

---

# 3. MC 层整体图

```text
┌─────────────────────────────────────────────────────────────┐
│                      TinyDSP MC Layer                       │
└─────────────────────────────────────────────────────────────┘

                 ┌──────────────────────┐
                 │ TinyDSPMCTargetDesc   │
                 │ 注册 MC 组件           │
                 └───────────┬──────────┘
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
          v                  v                  v
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│ MCAsmInfo         │ │ MCInstrInfo       │ │ MCRegisterInfo   │
│ 汇编语法属性       │ │ 指令元信息         │ │ 寄存器元信息       │
└──────────────────┘ └──────────────────┘ └──────────────────┘

          ┌──────────────────┬──────────────────┬──────────────────┐
          │                  │                  │
          v                  v                  v
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│ InstPrinter       │ │ MCCodeEmitter     │ │ AsmBackend       │
│ MCInst → asm      │ │ MCInst → binary   │ │ fixup/object/NOP │
└──────────────────┘ └──────────────────┘ └─────────┬────────┘
                                                     │
                                                     v
                                           ┌──────────────────┐
                                           │ ELFObjectWriter  │
                                           │ object/ELF 输出   │
                                           └──────────────────┘
```

可以理解为：

```text
TinyDSPMCTargetDesc.cpp 是 MC 层注册入口；
其他文件是被注册进去的具体 MC services。
```

---

# 4. TinyDSPMCTargetDesc：MC 层注册中心

`TinyDSPMCTargetDesc.cpp` 负责把 TinyDSP 的 MC 组件注册到 LLVM `TargetRegistry` 里。它注册了：

```text
MCAsmInfo
MCInstrInfo
MCRegisterInfo
MCSubtargetInfo
MCCodeEmitter
MCAsmBackend
MCInstPrinter
```

这些注册都在 `LLVMInitializeTinyDSPTargetMC()` 中完成。

```text
LLVMInitializeTinyDSPTargetMC()
  │
  ├── RegisterMCAsmInfo
  │       └── TinyDSPMCAsmInfo
  │
  ├── RegisterMCInstrInfo
  │       └── InitTinyDSPMCInstrInfo()
  │
  ├── RegisterMCRegInfo
  │       └── InitTinyDSPMCRegisterInfo()
  │
  ├── RegisterMCSubtargetInfo
  │       └── createTinyDSPMCSubtargetInfoImpl()
  │
  ├── RegisterMCCodeEmitter
  │       └── createTinyDSPMCCodeEmitter()
  │
  ├── RegisterMCAsmBackend
  │       └── createTinyDSPAsmBackend()
  │
  └── RegisterMCInstPrinter
          └── createTinyDSPMCInstPrinter()
```

这一步的意义是：

```text
LLVM tools such as llc / llvm-mc / llvm-objdump
    │
    │ need MC services for target "tinydsp"
    v
TargetRegistry
    │
    v
TinyDSP registered MC components
```

所以：

```text
llc 需要输出 .s / .o 时，会用 TinyDSP MC 层。
llvm-mc 需要汇编/编码 TinyDSP asm 时，也会用 TinyDSP MC 层。
```

---

# 5. TinyDSPMCTargetDesc.h：MC 层公共头文件

`TinyDSPMCTargetDesc.h` 主要做三件事：

```text
1. 声明创建 MCCodeEmitter / AsmBackend / ELFObjectWriter 的函数
2. include TableGen 生成的寄存器枚举
3. include TableGen 生成的指令枚举和 subtarget 枚举
```

它声明了：

```text
createTinyDSPMCCodeEmitter(...)
createTinyDSPAsmBackend(...)
createTinyDSPELFObjectWriter(...)
```

并通过 `TinyDSPGenRegisterInfo.inc`、`TinyDSPGenInstrInfo.inc`、`TinyDSPGenSubtargetInfo.inc` 提供 TinyDSP 的寄存器、指令和 subtarget 枚举。

图示：

```text
TinyDSP .td files
  │
  │ llvm-tblgen
  v
TinyDSPGenRegisterInfo.inc
TinyDSPGenInstrInfo.inc
TinyDSPGenSubtargetInfo.inc
  │
  v
TinyDSPMCTargetDesc.h
  │
  v
MC layer C++ files can use:
  - TinyDSP::R0
  - TinyDSP::ADD
  - TinyDSP::SUB
  - TinyDSP subtarget feature IDs
```

---

# 6. TinyDSPMCAsmInfo：汇编文件的基本语法属性

`TinyDSPMCAsmInfo.cpp` 描述 TinyDSP 汇编格式的一些基础属性。当前它设置了：

```text
IsLittleEndian = true
Data16bitsDirective = ".2byte"
Data32bitsDirective = ".4byte"
Data64bitsDirective = ".8byte"
PrivateLabelPrefix = ".L"
CommentString = "#"
SupportsDebugInformation = true
ExceptionsType = DwarfCFI
```

这些信息来自 `TinyDSPMCAsmInfo` 构造函数。

图示：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPMCAsmInfo                             │
├─────────────────────────────────────────────┤
│ 描述 TinyDSP assembly file 的基本格式         │
│                                             │
│ - little endian                              │
│ - .2byte / .4byte / .8byte directives        │
│ - private label prefix = .L                  │
│ - comment string = #                         │
│ - DWARF CFI debug/exception info             │
└─────────────────────────────────────────────┘
```

它回答的问题是：

```text
汇编文件里注释怎么写？
私有 label 前缀是什么？
数据 directive 用什么？
是否支持 debug info？
目标大小端是什么？
```

---

# 7. TinyDSPInstPrinter：MCInst → 汇编文本

`TinyDSPInstPrinter.cpp` 的职责是：

```text
MCInst → human-readable TinyDSP assembly
```

它 include 了 TableGen 生成的 `TinyDSPGenAsmWriter.inc`，然后实现了 `printInst()`、`printOperand()`、`printMemOperand()`。`printOperand()` 会打印寄存器、立即数或表达式；`printMemOperand()` 会按 `[base + offset]` 这种形式打印 memory operand。

图示：

```text
MCInst
  │
  │ TinyDSPInstPrinter
  v
TinyDSP assembly text
```

例子：

```text
MCInst opcode = ADD
operands = R0, R2, R3
        │
        v
ADD R0, R2, R3
```

memory operand：

```text
MCInst opcode = LOAD
operands = R0, R7, 4
        │
        v
LOAD R0, [R7 + 4]
```

它和 TableGen 的关系：

```text
TinyDSPInstrInfo.td
  │
  │ contains asm string / operands
  v
TinyDSPGenAsmWriter.inc
  │
  v
TinyDSPInstPrinter.cpp
  │
  v
printInstruction(...)
```

也就是说：

```text
大部分指令打印逻辑来自 TableGen 生成的 AsmWriter；
printOperand / printMemOperand 负责 TinyDSP 特定 operand 格式。
```

---

# 8. TinyDSPMCCodeEmitter：MCInst → 机器码

`TinyDSPMCCodeEmitter.cpp` 负责把 `MCInst` 编码成二进制机器指令。它实现了 `encodeInstruction()`，调用 `getBinaryCodeForInstr()` 得到 32-bit 指令编码，然后以 little-endian 写入输出 buffer。它还实现了 `getMachineOpValue()` 和 `encodeMemoryOpValue()`，用于把寄存器、立即数、memory operand 转成编码字段。

图示：

```text
MCInst
  │
  │ TinyDSPMCCodeEmitter
  v
32-bit binary instruction
  │
  v
little-endian bytes
```

更细：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPMCCodeEmitter                         │
├─────────────────────────────────────────────┤
│ encodeInstruction(MCInst)                    │
│   │                                         │
│   ├── getBinaryCodeForInstr()                │
│   │     └── TableGen generated encoding      │
│   │                                         │
│   ├── getMachineOpValue()                    │
│   │     ├── register → encoding value        │
│   │     ├── immediate → immediate bits       │
│   │     └── expr → currently returns 0       │
│   │                                         │
│   ├── encodeMemoryOpValue()                  │
│   │     └── base register + offset encoding  │
│   │                                         │
│   └── write little-endian bytes              │
└─────────────────────────────────────────────┘
```

例子：

```text
ADD R0, R2, R3
  │
  v
MCInst:
  opcode = ADD
  operands = R0, R2, R3
  │
  v
TinyDSPMCCodeEmitter
  │
  v
binary encoding
```

这里要注意：

```text
具体 bit layout 主要来自 TinyDSPInstrFormats.td / TinyDSPInstrInfo.td。
TinyDSPMCCodeEmitter.cpp 提供 operand encoding hook。
TinyDSPGenMCCodeEmitter.inc 提供 TableGen generated encoding logic。
```

---

# 9. TinyDSPAsmBackend：fixup / object backend / NOP

`TinyDSPAsmBackend.cpp` 是 MC 层里偏 object/assembler backend 的部分。

它当前做了几件事：

```text
1. 指定 little-endian MCAsmBackend
2. applyFixup() 目前为空
3. createObjectTargetWriter() 返回 TinyDSP ELF object writer
4. writeNopData() 写 4 字节 0 作为 NOP 数据
```

这些可以从 `TinyDSPAsmBackend` 实现看到：构造时使用 little-endian；`applyFixup` 为空；`createObjectTargetWriter()` 调用 `createTinyDSPELFObjectWriter(0)`；`writeNopData()` 要求字节数是 4 的倍数，并写入 `\x00\x00\x00\x00`。

图示：

```text
┌─────────────────────────────────────────────┐
│ TinyDSPAsmBackend                            │
├─────────────────────────────────────────────┤
│ 负责：                                      │
│ - target-specific assembler backend behavior │
│ - fixup application                          │
│ - object writer creation                     │
│ - NOP padding data                           │
└─────────────────────────────────────────────┘
```

当前状态可以准确描述为：

```text
TinyDSPAsmBackend 已经提供了 object writer 接入口和 NOP 写入；
relocation/fixup 逻辑还很 minimal，applyFixup() 目前没有实际处理。
```

---

# 10. TinyDSPELFObjectWriter：ELF object 输出

`TinyDSPELFObjectWriter.cpp` 负责 TinyDSP 的 ELF object writer。

当前实现中，`TinyDSPELFObjectWriter` 继承 `MCELFObjectTargetWriter`，配置为 32-bit ELF，machine type 当前使用 `ELF::EM_NONE`，并且 relocation 还没有真正实现，`getRelocType()` 返回 `ELF::R_386_NONE`。

图示：

```text
MCAssembler
  │
  │ asks AsmBackend for object writer
  v
TinyDSPELFObjectWriter
  │
  v
ELF object file
```

当前能力边界：

```text
已有：
  - ELF object writer skeleton
  - 32-bit object writer setup
  - 接入 MC object emission path

尚未完整：
  - TinyDSP-specific ELF machine type
  - TinyDSP-specific relocation types
  - real fixup/relocation handling
```

这不是缺点，而是一个很正常的 educational backend 阶段。可以说：

```text
The MC object emission path is scaffolded, while relocation and fixup handling remain minimal.
```

---

# 11. 从 MachineInstr 到 `.s` 的路径

这是 `clang -target tinydsp -S` 或 `llc -mtriple=tinydsp -o output.s` 的典型路径：

```text
Final MachineInstr
  │
  │ TinyDSPAsmPrinter.cpp
  v
MCInst
  │
  │ TinyDSPMCInstLower.cpp
  v
MCStreamer
  │
  │ TinyDSPInstPrinter
  v
TinyDSP assembly text
```

图示：

```text
┌──────────────────────────────┐
│ MachineInstr                  │
│ ADD %physreg0, %physreg2, ... │
└───────────────┬──────────────┘
                │
                │ TinyDSPMCInstLower
                v
┌──────────────────────────────┐
│ MCInst                        │
│ opcode = ADD                  │
│ operands = R0, R2, R3         │
└───────────────┬──────────────┘
                │
                │ TinyDSPInstPrinter
                v
┌──────────────────────────────┐
│ Assembly text                 │
│ ADD R0, R2, R3                │
└──────────────────────────────┘
```

这里的重点：

```text
MachineInstr 是 CodeGen 层对象。
MCInst 是 MC 层对象。
TinyDSPMCInstLower 是桥。
TinyDSPInstPrinter 把 MCInst 打印成汇编文本。
```

---

# 12. 从 MCInst 到 `.o` 的路径

如果输出 object file，路径会变成：

```text
MCInst
  │
  │ MCCodeEmitter
  v
binary instruction bytes
  │
  │ AsmBackend
  v
fixup / relaxation / NOP / object backend
  │
  │ ELFObjectWriter
  v
TinyDSP object file / ELF
```

图示：

```text
┌──────────────────────────────┐
│ MCInst                        │
│ ADD R0, R2, R3                │
└───────────────┬──────────────┘
                │
                │ TinyDSPMCCodeEmitter
                v
┌──────────────────────────────┐
│ Encoded instruction           │
│ 32-bit binary value           │
│ little-endian bytes           │
└───────────────┬──────────────┘
                │
                │ TinyDSPAsmBackend
                v
┌──────────────────────────────┐
│ Assembler backend             │
│ fixups / NOP / object writer  │
└───────────────┬──────────────┘
                │
                │ TinyDSPELFObjectWriter
                v
┌──────────────────────────────┐
│ ELF object file               │
└──────────────────────────────┘
```

---

# 13. MC 层和 TableGen 的关系

MC 层大量依赖 TableGen 生成文件。

```text
TinyDSP .td files
  │
  │ llvm-tblgen
  v
TinyDSPGenInstrInfo.inc
TinyDSPGenRegisterInfo.inc
TinyDSPGenSubtargetInfo.inc
TinyDSPGenAsmWriter.inc
TinyDSPGenMCCodeEmitter.inc
  │
  v
TinyDSP MC layer
```

对应关系：

```text
TinyDSPGenInstrInfo.inc
  → MCInstrInfo
  → 指令 opcode、operand、属性

TinyDSPGenRegisterInfo.inc
  → MCRegisterInfo
  → 寄存器编号、编码值、寄存器名

TinyDSPGenSubtargetInfo.inc
  → MCSubtargetInfo
  → CPU/features/scheduling 相关信息

TinyDSPGenAsmWriter.inc
  → TinyDSPInstPrinter
  → MCInst 打印成汇编

TinyDSPGenMCCodeEmitter.inc
  → TinyDSPMCCodeEmitter
  → MCInst 编码成机器码
```

图示：

```text
┌──────────────────────────────┐
│ Declarative TableGen side     │
├──────────────────────────────┤
│ registers                     │
│ instructions                  │
│ instruction formats            │
│ asm strings                   │
│ encoding bits                 │
│ subtarget info                │
└──────────────┬───────────────┘
               │ tablegen
               v
┌──────────────────────────────┐
│ Generated MC metadata         │
├──────────────────────────────┤
│ TinyDSPGenInstrInfo.inc       │
│ TinyDSPGenRegisterInfo.inc    │
│ TinyDSPGenAsmWriter.inc       │
│ TinyDSPGenMCCodeEmitter.inc   │
└──────────────┬───────────────┘
               │ included by
               v
┌──────────────────────────────┐
│ MC C++ implementation         │
├──────────────────────────────┤
│ MCTargetDesc.cpp              │
│ InstPrinter.cpp               │
│ MCCodeEmitter.cpp             │
│ AsmBackend.cpp                │
│ ELFObjectWriter.cpp           │
└──────────────────────────────┘
```

---

# 14. MC 层和 CodeGen 层的边界

这个边界非常重要：

```text
┌─────────────────────────────────────────────────────────────┐
│ CodeGen Layer                                                │
├─────────────────────────────────────────────────────────────┤
│ 输入：LLVM IR / SelectionDAG                                 │
│ 输出：MachineInstr                                           │
│                                                             │
│ 主要文件：                                                   │
│ - TinyDSPISelLowering.cpp                                    │
│ - TinyDSPISelDAGToDAG.cpp                                    │
│ - TinyDSPInstrInfo.cpp                                       │
│ - TinyDSPRegisterInfo.cpp                                    │
│ - TinyDSPFrameLowering.cpp                                   │
│ - TinyDSPAsmPrinter.cpp                                      │
│ - TinyDSPMCInstLower.cpp                                     │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       │ MachineInstr → MCInst
                       v
┌─────────────────────────────────────────────────────────────┐
│ MC Layer                                                     │
├─────────────────────────────────────────────────────────────┤
│ 输入：MCInst / MCOperand                                     │
│ 输出：assembly text / encoded bytes / object file            │
│                                                             │
│ 主要文件：                                                   │
│ - TinyDSPMCTargetDesc.cpp                                    │
│ - TinyDSPMCAsmInfo.cpp                                       │
│ - TinyDSPInstPrinter.cpp                                     │
│ - TinyDSPMCCodeEmitter.cpp                                   │
│ - TinyDSPAsmBackend.cpp                                      │
│ - TinyDSPELFObjectWriter.cpp                                 │
└─────────────────────────────────────────────────────────────┘
```

一句话：

```text
CodeGen 还关心 LLVM machine-level 语义；
MC 层已经不关心 LLVM IR，也不关心 SelectionDAG，只关心 MCInst 如何变成汇编/机器码/object。
```

---

# 15. 对 `llc` / `clang` / `llvm-mc` 的作用

## `llc -mtriple=tinydsp input.ll -o output.s`

```text
LLVM IR
  ↓
TinyDSP CodeGen
  ↓
MachineInstr
  ↓
AsmPrinter / MCInstLower
  ↓
MCInst
  ↓
TinyDSPInstPrinter
  ↓
output.s
```

## `llc -mtriple=tinydsp input.ll -filetype=obj -o output.o`

```text
LLVM IR
  ↓
TinyDSP CodeGen
  ↓
MachineInstr
  ↓
MCInst
  ↓
TinyDSPMCCodeEmitter
  ↓
TinyDSPAsmBackend
  ↓
TinyDSPELFObjectWriter
  ↓
output.o
```

## `llvm-mc -triple=tinydsp input.s`

```text
TinyDSP assembly
  ↓
LLVM MC parser path
  ↓
MCInst
  ↓
TinyDSPMCCodeEmitter
  ↓
encoded bytes / object
```

不过这里要注意一个边界：当前 MC 层明显有 InstPrinter、CodeEmitter、AsmBackend、ELF writer skeleton；至于完整 asm parser 是否已经可用，要看是否实现了 TinyDSP asm parser。现在这个 `MCTargetDesc/` 主要展示的是 **打印/编码/object 输出方向**，不是完整汇编解析器方向。

---

# 16. 当前 TinyDSP MC 层能力总结

```text
┌──────────────────────────────┬──────────────────────────────┐
│ MC 组件                       │ 当前作用                     │
├──────────────────────────────┼──────────────────────────────┤
│ TinyDSPMCTargetDesc.cpp       │ 注册 MC 组件                 │
│ TinyDSPMCAsmInfo.cpp          │ 定义汇编语法属性             │
│ TinyDSPInstPrinter.cpp        │ MCInst → assembly text       │
│ TinyDSPMCCodeEmitter.cpp      │ MCInst → 32-bit binary       │
│ TinyDSPAsmBackend.cpp         │ object backend / NOP / fixup │
│ TinyDSPELFObjectWriter.cpp    │ ELF object writer skeleton   │
└──────────────────────────────┴──────────────────────────────┘
```

更准确的成熟度表达：

```text
已经实现：
  - MC target registration
  - assembly printing
  - MC instruction encoding skeleton
  - little-endian 32-bit encoding output
  - memory operand encoding
  - assembler backend skeleton
  - ELF object writer skeleton

还可以增强：
  - relocation/fixup handling
  - TinyDSP-specific ELF machine type
  - TinyDSP relocation types
  - full asm parser if needed
  - objdump/disassembler support if needed
```

---

# 17. 总结

英文可以这样说：

> The TinyDSP MC layer is responsible for the low-level representation and emission of TinyDSP instructions after CodeGen. CodeGen produces `MachineInstr`, then `TinyDSPMCInstLower` converts it to `MCInst`. The MC layer registers TinyDSP-specific MC services through `LLVMInitializeTinyDSPTargetMC`, including instruction info, register info, subtarget info, instruction printer, code emitter, asm backend, and ELF object writer. `TinyDSPInstPrinter` prints `MCInst` as assembly text, while `TinyDSPMCCodeEmitter` encodes `MCInst` into 32-bit little-endian machine instructions. `TinyDSPAsmBackend` and `TinyDSPELFObjectWriter` provide the object emission path, although relocation and fixup handling are still minimal.

中文可以这样说：

> TinyDSP MC 层负责 CodeGen 之后的底层指令表示和输出。CodeGen 生成 `MachineInstr`，再由 `TinyDSPMCInstLower` 转成 `MCInst`。MC 层通过 `LLVMInitializeTinyDSPTargetMC` 注册 TinyDSP 的 MCInstrInfo、MCRegisterInfo、MCSubtargetInfo、InstPrinter、MCCodeEmitter、AsmBackend 和 ELFObjectWriter。`TinyDSPInstPrinter` 负责把 `MCInst` 打印成汇编文本，`TinyDSPMCCodeEmitter` 负责把 `MCInst` 编码成 32-bit little-endian 机器码，`TinyDSPAsmBackend` 和 `TinyDSPELFObjectWriter` 提供 object/ELF 输出路径。目前 relocation/fixup 处理还比较 minimal，适合继续扩展。

最核心图：

```text
MachineInstr
  ↓ TinyDSPMCInstLower
MCInst
  ├── TinyDSPInstPrinter
  │       ↓
  │   assembly text
  │
  └── TinyDSPMCCodeEmitter
          ↓
      encoded bytes
          ↓
      TinyDSPAsmBackend
          ↓
      TinyDSPELFObjectWriter
          ↓
      ELF object file
```
