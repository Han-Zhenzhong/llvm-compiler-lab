# TinyDSP LLVM Backend

A minimal DSP-like target backend for LLVM, designed for learning compiler backend development and hardware-software co-design.

## Overview

TinyDSP is a simplified 32-bit RISC-style DSP architecture with:
- **8 general-purpose registers** (R0-R7)
- **Simple instruction set**: ADD, SUB, MUL, LOAD, STORE, RET, and logical operations
- **32-bit instruction encoding**
- **Little-endian memory model**
- **Single-issue pipeline model**

## Architecture Specifications

### Registers
| Register | Purpose | Preserved across calls |
|----------|---------|------------------------|
| R0-R1 | Return values / temporaries | No |
| R2-R5 | Function arguments / temporaries | No |
| R6 | Frame pointer (FP) | Yes |
| R7 | Stack pointer (SP) | Yes |

### Instruction Set

#### Arithmetic Instructions
- **ADD** rd, rs1, rs2 - rd = rs1 + rs2
- **SUB** rd, rs1, rs2 - rd = rs1 - rs2
- **MUL** rd, rs1, rs2 - rd = rs1 * rs2
- **ADDI** rd, rs1, imm18 - rd = rs1 + sign_extend(imm18)

#### Logical Instructions
- **AND** rd, rs1, rs2 - rd = rs1 & rs2
- **OR** rd, rs1, rs2 - rd = rs1 | rs2
- **XOR** rd, rs1, rs2 - rd = rs1 ^ rs2

#### Memory Instructions
- **LOAD** rd, [base + offset] - rd = mem[base + offset]
- **STORE** rs, [base + offset] - mem[base + offset] = rs

#### Control Flow
- **RET** - Return from function

### Instruction Encoding

#### R-Type (Register-Register)
```
31      24 23  21 20  18 17  15 14           0
+----------+------+------+------+---------------+
| opcode   |  rd  | rs1  | rs2  |   unused      |
+----------+------+------+------+---------------+
```

#### I-Type (Immediate)
```
31      24 23  21 20  18 17                  0
+----------+------+------+---------------------+
| opcode   |  rd  | rs1  |    imm18            |
+----------+------+------+---------------------+
```

#### M-Type (Memory)
```
31      24 23  21 20  18 17                  0
+----------+------+------+---------------------+
| opcode   | reg  | base |    offset           |
+----------+------+------+---------------------+
```

### Scheduling Model
- **Issue Width**: 1 instruction per cycle
- **ALU Operations**: 1 cycle latency
- **Multiply**: 3 cycles latency
- **Load**: 2 cycles latency
- **Store**: 1 cycle latency

## Building

### Prerequisites
- LLVM source tree (version 19.x or later)
- CMake 3.20+
- C++17 compatible compiler

### Integration Steps

1. **Add to LLVM build configuration:**

```bash
cd llvm-project
mkdir build && cd build
cmake -G Ninja \
  -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" \
  -DCMAKE_BUILD_TYPE=Release \
  ../llvm
ninja
```

2. **Verify target is registered:**
```bash
./bin/llc --version | grep TinyDSP
```

## Usage

### Current Output and Linking Status

TinyDSP support is currently intended for assembly generation and simulator-based
validation.

- Fully supported workflow: compile C to `.s` and run verification with the
    Python simulator.
- Partially supported: generating `.o` files for experimentation.
- Not yet supported as a complete toolchain: producing linked TinyDSP
    executables with a TinyDSP-aware linker/runtime flow.

Notes:
- The backend currently uses placeholder ELF/relocation behavior in parts of
    the MC object writer path.
- `lld` does not currently provide a TinyDSP target implementation in this
    tree.

### Compiling C to TinyDSP Assembly

```bash
# Compile C code to TinyDSP assembly
clang -target tinydsp -S test.c -o test.s

# Compile with optimization
clang -target tinydsp -S -O2 test.c -o test_opt.s

# Generate object file
clang -target tinydsp -c test.c -o test.o
```

### How to inspect your generated test.o

After generating an object file, inspect its structure and contents with LLVM
binary utilities:

```bash
# 1) ELF header (class, machine, file type)
llvm-readelf -h test.o

# 2) Relocation entries (if any)
llvm-readelf -r test.o

# 3) Symbol table (functions/objects/externals)
llvm-readelf -s test.o

# 4) Disassembly of code sections
llvm-objdump -d test.o
```

These commands are useful for checking whether code bytes, symbols, and
relocation information match expectations during backend bring-up.

### Example

**Input (test.c):**
```c
int add(int a, int b) {
    return a + b;
}
```

**Output (test.s):**
```asm
add:
    add r0, r2, r3
    ret
```

## Testing and Verification

### Run Simulator Tests

The simulator tests basic instruction execution:

```bash
cd llvm/utils/TinyDSP
python tinydsp_simulator.py
```

### Compile and Verify C Code

```bash
cd llvm/utils/TinyDSP
python verify_tinydsp.py ../../build

# Expected output:
# ✓ test_add(5, 3) = 8
# ✓ test_sub(10, 3) = 7
# ✓ test_mul(4, 5) = 20
# ✅ All tests PASSED!
```

## Project Structure

```
TinyDSP/
├── CMakeLists.txt              # Build configuration
├── TinyDSP.td                  # Target description root
├── TinyDSPRegisterInfo.td      # Register definitions
├── TinyDSPInstrInfo.td         # Instruction definitions
├── TinyDSPInstrFormats.td      # Instruction encoding formats
├── TinyDSPCallingConv.td       # Calling convention
├── TinyDSPSchedule.td          # Pipeline scheduling model
├── TinyDSP.h                   # Target interface
├── TinyDSPTargetMachine.{h,cpp}  # Target machine implementation
├── TinyDSPSubtarget.{h,cpp}      # Subtarget information
├── TinyDSPInstrInfo.{h,cpp}      # Instruction info
├── TinyDSPRegisterInfo.{h,cpp}   # Register info
├── TinyDSPFrameLowering.{h,cpp}  # Stack frame management
├── TinyDSPISelLowering.{h,cpp}   # Instruction selection lowering
├── TinyDSPISelDAGToDAG.cpp       # DAG-to-DAG instruction selector
├── MCTargetDesc/                 # Machine code description
│   ├── TinyDSPMCTargetDesc.{h,cpp}
│   ├── TinyDSPMCAsmInfo.{h,cpp}
│   ├── TinyDSPInstPrinter.{h,cpp}
│   ├── TinyDSPMCCodeEmitter.cpp
│   ├── TinyDSPAsmBackend.cpp
│   └── TinyDSPELFObjectWriter.cpp
├── TargetInfo/                   # Target registration
│   └── TinyDSPTargetInfo.cpp
├── tools/                        # Support tools
│   └── tinydsp_simulator.py     # Instruction simulator
└── test/                         # Test cases
    ├── test_basic.c              # Basic C test cases
    └── verify_tinydsp.py         # Verification script
```

## Development Guide

### Adding New Instructions

1. **Define in TinyDSPInstrInfo.td:**
```tablegen
def SUBI : IType<0x12, (outs GPR:$rd), (ins GPR:$rs1, simm18:$imm),
                 "subi\t$rd, $rs1, $imm",
                 [(set i32:$rd, (sub i32:$rs1, simm18:$imm))]>;
```

2. **Update simulator** in `llvm/utils/TinyDSP/tinydsp_simulator.py`

3. **Add test case** in `llvm/test/CodeGen/TinyDSP/Inputs/test_basic.c`

### Extending the Architecture

- **Add registers**: Modify `TinyDSPRegisterInfo.td`
- **Add instruction formats**: Update `TinyDSPInstrFormats.td`
- **Modify calling convention**: Edit `TinyDSPCallingConv.td`
- **Adjust scheduling**: Update `TinyDSPSchedule.td`

## Learning Resources

This backend serves as an educational tool for:

1. **LLVM Backend Development**
   - TableGen instruction descriptions
   - Instruction selection (DAG-to-DAG)
   - Register allocation
   - Frame lowering and calling conventions

2. **Hardware-Software Co-Design**
   - ISA design trade-offs
   - Compiler optimizations impact
   - Instruction scheduling
   - Performance analysis

3. **Verification**
   - Simulation-based testing
   - Compiler output validation
   - Cross-verification between compiler and simulator

## Extending to Real Hardware

To connect this backend to real RTL:

1. **Generate Machine Code**: Use the MCCodeEmitter
2. **Create Test Harness**: Write RTL testbench that loads generated code
3. **Co-Simulation**: Run same tests in simulator and RTL
4. **Verify Equivalence**: Compare register/memory state

Example workflow:
```bash
# Generate binary
clang -target tinydsp -c test.c -o test.o
llvm-objdump -d test.o > test.bin

# Load in RTL simulator (Verilator/ModelSim)
# Compare execution traces
```

## Known Limitations

- No support for floating point operations
- No SIMD/vector instructions
- Basic optimization (no advanced peephole optimizations)
- Limited debug information generation
- No inline assembly support
- Object emission support is incomplete for production use (ELF machine/reloc
    details are still placeholder-level)
- End-to-end linked TinyDSP executables are not currently supported in this
    tree

## Future Enhancements

- [ ] Add SIMD/DSP-specific instructions (MAC, saturating arithmetic)
- [ ] Implement delay slots and branch prediction
- [ ] Add more sophisticated scheduling model
- [ ] Support for multiple pipeline stages
- [ ] Integration with FPGA development flow
- [ ] Generate cycle-accurate performance reports

## Contributing

This is an educational project. Contributions that improve learning value are welcome:
- Additional test cases
- Documentation improvements
- Example programs
- Integration with RTL simulators

## License

Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
See LICENSE.TXT for details.

## References

- [LLVM Backend Development Tutorial](https://llvm.org/docs/WritingAnLLVMBackend.html)
- [TableGen Guide](https://llvm.org/docs/TableGen/)
- [LLVM Compiler Infrastructure](https://llvm.org/)

## Contact

For questions about this backend, refer to the main development plan document:
`docs/tidydsp_dev_plan.md`
