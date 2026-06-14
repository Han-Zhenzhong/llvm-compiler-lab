# Overall

## High Level Overall

```text
TinyDSP source files
   ↓
CMake builds TinyDSP target libraries
   ↓
LLVM target registry registers "tinydsp"
   ↓
LLVM tools call InitializeAllTargets / InitializeAllTargetMC
   ↓
-target tinydsp or -mtriple=tinydsp selects TinyDSP
   ↓
TinyDSP TargetMachine / MC layer is used
```

## Overall Detail

```text
┌─────────────────────────────────────────────┐
│ Existing LLVM tools                         │
│                                             │
│ clang                                       │
│ llc                                         │
│ llvm-mc                                     │
│ llvm-objdump                                │
└──────────────────────┬──────────────────────┘
                       │
                       │ uses LLVM target registry
                       v
┌─────────────────────────────────────────────┐
│ LLVM Target Registry                        │
│                                             │
│ target name: tinydsp                        │
│ target desc: TinyDSP                        │
│ target object: getTheTinyDSPTarget()        │
└──────────────────────┬──────────────────────┘
                       │
                       │ dispatches by target triple
                       v
┌─────────────────────────────────────────────┐
│ TinyDSP backend libraries                   │
│                                             │
│ LLVMTinyDSPInfo                             │
│ LLVMTinyDSPDesc                             │
│ LLVMTinyDSPCodeGen                          │
└──────────────────────┬──────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────┐
│ TinyDSP implementation                      │
│                                             │
│ TargetInfo                                  │
│ TargetMachine                               │
│ ISelLowering                                │
│ DAGToDAG instruction selection              │
│ RegisterInfo                                │
│ InstrInfo                                   │
│ FrameLowering                               │
│ AsmPrinter                                  │
│ MCCodeEmitter / AsmBackend / InstPrinter    │
└─────────────────────────────────────────────┘
```

# CMake Overall

```text
llvm/lib/Target/CMakeLists.txt
   │
   │ foreach(t ${LLVM_TARGETS_TO_BUILD})
   │   add_subdirectory(${t})
   │
   v
llvm/lib/Target/TinyDSP/CMakeLists.txt
   │
   ├── TableGen generated files
   │     ├── TinyDSPGenInstrInfo.inc
   │     ├── TinyDSPGenRegisterInfo.inc
   │     ├── TinyDSPGenDAGISel.inc
   │     ├── TinyDSPGenAsmWriter.inc
   │     └── TinyDSPGenMCCodeEmitter.inc
   │
   ├── TinyDSPCodeGen library
   │     ├── TinyDSPTargetMachine.cpp
   │     ├── TinyDSPISelLowering.cpp
   │     ├── TinyDSPISelDAGToDAG.cpp
   │     ├── TinyDSPInstrInfo.cpp
   │     ├── TinyDSPRegisterInfo.cpp
   │     ├── TinyDSPFrameLowering.cpp
   │     └── TinyDSPAsmPrinter.cpp
   │
   ├── TargetInfo/
   │
   └── MCTargetDesc/
```

# Tools Calling Flow

```text
User command:
  llc -mtriple=tinydsp input.ll

LLVM Triple parser:
  "tinydsp" → llvm::Triple::tinydsp

LLVM target lookup:
  Triple arch = tinydsp
  ↓
  find registered target named "tinydsp"
```

# TinyDSP backend processing flow

```text
llc / clang backend
   ↓
TargetRegistry::lookupTarget("tinydsp")
   ↓
create TinyDSPTargetMachine
   ↓
TinyDSPPassConfig
   ↓
addInstSelector()
   ↓
createTinyDSPISelDag()
   ↓
SelectionDAG → TinyDSP MachineInstr
```

The fifth integration point is the **MC layer**. This is used by tools that need assembly printing, object emission, instruction encoding, or assembly-related functionality. Your `TinyDSPMCTargetDesc.cpp` registers MC components including instruction info, register info, subtarget info, code emitter, asm backend, and instruction printer. ([GitHub][5])

```text
LLVMInitializeTinyDSPTargetMC()
   │
   ├── RegisterMCInstrInfo
   ├── RegisterMCRegInfo
   ├── RegisterMCSubtargetInfo
   ├── RegisterMCCodeEmitter
   ├── RegisterMCAsmBackend
   └── RegisterMCInstPrinter
```

# Overall processing

So the flow for `llc` is:

```text
input.ll
  │
  │ llc -mtriple=tinydsp
  v
LLVM IR Module
  │
  v
TargetRegistry lookup
  │
  v
TinyDSPTargetMachine
  │
  v
TinyDSPISelLowering
  │
  v
TinyDSPISelDAGToDAG
  │
  v
TinyDSP MachineInstr
  │
  v
TinyDSPAsmPrinter
  │
  v
TinyDSP assembly
```

The flow for `clang` is similar, but starts earlier:

```text
test.c
  │
  │ clang -target tinydsp -S
  v
Clang driver accepts target
  │
  v
Clang frontend generates LLVM IR
  │
  v
LLVM backend selects TinyDSPTargetMachine
  │
  v
TinyDSP backend lowers IR to assembly
  │
  v
test.s
```

The flow for `llvm-mc` is more MC-layer focused:

```text
TinyDSP assembly
  │
  │ llvm-mc -triple=tinydsp
  v
MC parser / MC layer
  │
  v
TinyDSPInstPrinter / TinyDSPMCCodeEmitter
  │
  v
encoded instructions / object output
```

# How TinyDSP is linked into LLVM tools

TinyDSP is integrated as a normal LLVM target backend. The build system adds
`llvm/lib/Target/TinyDSP` when `TinyDSP` is included in `LLVM_TARGETS_TO_BUILD`.
The target is registered through `LLVMInitializeTinyDSPTargetInfo`, which exposes
the target name `tinydsp` to LLVM's target registry. The code generator is
registered through `LLVMInitializeTinyDSPTarget`, which allows tools such as
`llc` and Clang's backend pipeline to create `TinyDSPTargetMachine`.

The MC layer is registered through `LLVMInitializeTinyDSPTargetMC`, which provides
instruction information, register information, subtarget information, assembly
printing, instruction encoding, and object emission support.

As a result, existing LLVM tools can use TinyDSP through normal target selection:

```bash
llc -mtriple=tinydsp input.ll -o output.s
clang -target tinydsp -S input.c -o output.s
llvm-mc -triple=tinydsp input.s
````

TinyDSP is therefore not a standalone compiler. It is a backend plugged into the
existing LLVM toolchain architecture.

The strongest mental model is:

```text
This experiment added a backend.
This experiment did not rewrite the tools.

Existing LLVM tools are generic drivers.
TinyDSP provides target-specific services.
The registry connects the two.
````

Simple explanation:

> TinyDSP is linked into existing LLVM tools through LLVM’s target registry. This experiment added TinyDSP as a normal backend under `llvm/lib/Target`, registered its target info, target machine, and MC layer, and extended target triple recognition. After that, tools such as `llc`, `clang`, and `llvm-mc` can select TinyDSP through `-mtriple=tinydsp` or `-target tinydsp`, then dispatch into my TinyDSP lowering, instruction selection, asm printer, and MC components.
