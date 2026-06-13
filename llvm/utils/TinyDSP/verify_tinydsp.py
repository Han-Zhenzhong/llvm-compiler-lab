#!/usr/bin/env python3
"""
TinyDSP Compilation and Verification Script

This script:
1. Compiles C code to TinyDSP assembly using LLVM
2. Simulates the assembly execution
3. Verifies correctness
"""

import subprocess
import sys
from pathlib import Path

# Add simulator to path (same directory as this script).
sys.path.insert(0, str(Path(__file__).parent))
from tinydsp_simulator import TinyDSPSimulator


def compile_to_asm(c_file: str, output_asm: str, llvm_build_dir: str) -> bool:
    """Compile C file to TinyDSP assembly."""
    clang_path = Path(llvm_build_dir) / "bin" / "clang"
    
    if not clang_path.exists():
        print(f"❌ clang not found at {clang_path}")
        return False
    
    cmd = [
        str(clang_path),
        "-target", "tinydsp",
        "-S",
        c_file,
        "-o", output_asm,
        "-O1"
    ]
    
    print(f"🔨 Compiling: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"❌ Compilation failed:")
            print(result.stderr)
            return False
        print(f"✓ Compiled successfully to {output_asm}")
        return True
    except Exception as e:
        print(f"❌ Compilation error: {e}")
        return False


def extract_function_asm(asm_file: str, function_name: str) -> str:
    """Extract assembly for a specific function."""
    with open(asm_file, 'r') as f:
        content = f.read()
    
    # Find function boundaries
    func_pattern = f"{function_name}:"
    func_start = content.find(func_pattern)
    
    if func_start == -1:
        return None
    
    # Extract until next function or end
    lines = content[func_start:].split('\n')
    func_lines = []
    
    for line in lines[1:]:  # Skip function label
        if line.strip().startswith('.'):
            # Skip directives
            continue
        if ':' in line and not line.strip().startswith('#'):
            # Next function found
            break
        func_lines.append(line)
    
    return '\n'.join(func_lines)


def test_function(sim: TinyDSPSimulator, func_asm: str, 
                  args: list, expected: int, func_name: str) -> bool:
    """Test a function with given arguments."""
    sim.reset()
    
    # Set up arguments in R2-R5 (calling convention)
    for i, arg in enumerate(args):
        if i < 4:
            sim.regs[2 + i] = arg
    
    # Set up stack pointer
    sim.regs[7] = 0x10000
    
    try:
        sim.run_asm(func_asm)
        result = sim.get_register(0)  # Return value in R0
        
        if result == expected:
            print(f"  ✓ {func_name}{tuple(args)} = {result}")
            return True
        else:
            print(f"  ❌ {func_name}{tuple(args)}: expected {expected}, got {result}")
            sim.print_state()
            return False
    except Exception as e:
        print(f"  ❌ {func_name} execution error: {e}")
        return False


def main():
    if len(sys.argv) < 2:
        print("Usage: python verify_tinydsp.py <llvm_build_directory>")
        print("Example: python verify_tinydsp.py ../../build")
        sys.exit(1)
    
    llvm_build_dir = sys.argv[1]
    repo_root = Path(__file__).resolve().parents[2]
    c_file = repo_root / "llvm" / "test" / "CodeGen" / "TinyDSP" / "Inputs" / "test_basic.c"
    asm_file = Path(__file__).parent / "test_basic.s"

    if not c_file.exists():
        print(f"Input C file not found: {c_file}")
        sys.exit(1)
    
    print("=" * 60)
    print("TinyDSP Backend Verification")
    print("=" * 60)
    
    # Step 1: Compile
    if not compile_to_asm(str(c_file), str(asm_file), llvm_build_dir):
        sys.exit(1)
    
    # Step 2: Extract and test functions
    print("\n📋 Testing functions...")
    
    sim = TinyDSPSimulator()
    all_passed = True
    
    tests = [
        ("test_add", [5, 3], 8),
        ("test_add", [100, 50], 150),
        ("test_sub", [10, 3], 7),
        ("test_sub", [100, 25], 75),
        ("test_mul", [4, 5], 20),
        ("test_mul", [7, 8], 56),
        ("test_complex", [3, 4], 13),  # (3+4)*2-1 = 13
    ]
    
    for func_name, args, expected in tests:
        func_asm = extract_function_asm(str(asm_file), func_name)
        if func_asm is None:
            print(f"  ❌ Function {func_name} not found in assembly")
            all_passed = False
            continue
        
        if not test_function(sim, func_asm, args, expected, func_name):
            all_passed = False
    
    print("\n" + "=" * 60)
    if all_passed:
        print("✅ All tests PASSED!")
        return 0
    else:
        print("❌ Some tests FAILED")
        return 1


if __name__ == "__main__":
    sys.exit(main())
