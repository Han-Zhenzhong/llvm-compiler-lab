# TinyDSP Backend - Quick Start Guide

## 快速开始指南

本文档提供快速构建和测试 TinyDSP LLVM 后端的步骤。

## 前置条件

- Windows/Linux/macOS 系统
- CMake 3.20+
- Ninja 或 Make
- Python 3.7+
- C++17 编译器 (GCC 7+, Clang 5+, MSVC 2019+)

## 构建步骤

### 1. 配置 LLVM 构建

在 LLVM 项目根目录下执行:

```bash
cd llvm-project
mkdir build
cd build

# Windows (使用 Ninja)
cmake -G Ninja ^
  -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_ENABLE_ASSERTIONS=ON ^
  ..\llvm

# Linux/macOS
cmake -G Ninja \
  -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  ../llvm
```

### 2. 编译 TinyDSP 后端

```bash
# 编译整个 LLVM (包括 TinyDSP)
ninja

# 或者只编译 TinyDSP 相关组件
ninja TinyDSPCodeGen TinyDSPDesc TinyDSPInfo

# 验证 TinyDSP 目标已注册
./bin/llc --version | grep -i tinydsp
# 应该看到: TinyDSP
```

### 3. 测试编译器

#### 测试 1: 编译简单 C 程序

创建测试文件 `test.c`:
```c
int add(int a, int b) {
    return a + b;
}
```

编译为 TinyDSP 汇编:
```bash
./bin/clang -target tinydsp -S test.c -o test.s
cat test.s
```

预期输出类似:
```asm
add:
    add r0, r2, r3
    ret
```

#### 测试 2: 使用模拟器验证

```bash
cd llvm/utils/TinyDSP
python tinydsp_simulator.py
```

预期输出:
```
TinyDSP Simulator - Running tests...

✓ Test simple_add passed
✓ Test memory passed
✓ Test arithmetic passed

✅ All tests passed!
```

#### 测试 3: 完整验证流程

```bash
cd llvm/utils/TinyDSP
python verify_tinydsp.py ../../build
```

预期输出:
```
============================================================
TinyDSP Backend Verification
============================================================
🔨 Compiling: ...
✓ Compiled successfully to test_basic.s

📋 Testing functions...
  ✓ test_add(5, 3) = 8
  ✓ test_add(100, 50) = 150
  ✓ test_sub(10, 3) = 7
  ✓ test_mul(4, 5) = 20
  ✓ test_complex(3, 4) = 13

============================================================
✅ All tests PASSED!
```

## 快速测试示例

### 示例 1: 简单算术

```c
// arithmetic.c
int calculate(int x, int y) {
    int sum = x + y;
    int product = x * y;
    return sum + product;
}
```

编译并查看汇编:
```bash
./bin/clang -target tinydsp -S -O1 arithmetic.c -o arithmetic.s
cat arithmetic.s
```

### 示例 2: 循环

```c
// loop.c
int sum_array(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}
```

### 示例 3: 条件语句

```c
// branch.c
int max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}
```

## 常见问题

### Q1: 编译时找不到 TinyDSP 目标

**A:** 确保在 CMake 配置时添加了 `-DLLVM_TARGETS_TO_BUILD="...;TinyDSP"`

重新配置:
```bash
cd build
cmake -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" ../llvm
ninja
```

### Q2: clang 无法识别 -target tinydsp

**A:** 检查是否使用了正确的 clang 可执行文件:
```bash
./bin/clang --version
./bin/clang -print-targets | grep -i tinydsp
```

### Q3: 模拟器测试失败

**A:** 确保 Python 版本 >= 3.7:
```bash
python --version
# 或者
python3 --version
```

### Q4: TableGen 错误

**A:** 检查 TableGen 文件语法:
```bash
./bin/llvm-tblgen -I ../llvm/lib/Target/TinyDSP \
  ../llvm/lib/Target/TinyDSP/TinyDSP.td
```

## 调试技巧

### 查看 IR 到汇编的转换

```bash
# 查看 LLVM IR
./bin/clang -target tinydsp -S -emit-llvm test.c -o test.ll

# 查看 SelectionDAG
./bin/clang -target tinydsp -S test.c -mllvm -debug-only=isel -o test.s 2>&1 | less

# 查看指令选择详情
./bin/clang -target tinydsp -S test.c -mllvm -print-after-all -o test.s 2>&1 | less
```

### 使用 llc 直接编译

```bash
# 先生成 IR
./bin/clang -target tinydsp -S -emit-llvm test.c -o test.ll

# 使用 llc 编译
./bin/llc -march=tinydsp test.ll -o test.s

# 查看机器指令
./bin/llc -march=tinydsp -print-machineinstrs test.ll
```

### 模拟器调试模式

修改 `tinydsp_simulator.py`，在执行后打印状态:
```python
sim.run_asm(asm)
sim.print_state()  # 添加这行
```

## 性能分析

### 查看指令调度

```bash
./bin/llvm-mca -march=tinydsp test.s
```

### 查看寄存器分配

```bash
./bin/clang -target tinydsp -S test.c -mllvm -print-regalloc -o test.s 2>&1 | less
```

## 下一步

1. **阅读完整文档**: `llvm/lib/Target/TinyDSP/README.md`
2. **学习后端架构**: 查看 `docs/tidydsp_dev_plan.md`
3. **添加新指令**: 参考 README.md 中的 "Adding New Instructions"
4. **扩展架构**: 尝试添加 SIMD 或 DSP 特定指令

## 支持

遇到问题？

1. 检查 `llvm/lib/Target/TinyDSP/README.md` 的 Known Limitations 部分
2. 查看 LLVM 官方文档: https://llvm.org/docs/
3. 参考其他 LLVM 后端实现 (Lanai, MSP430)

## 学习资源

- **LLVM Backend 开发**: https://llvm.org/docs/WritingAnLLVMBackend.html
- **TableGen 语言**: https://llvm.org/docs/TableGen/
- **代码生成**: https://llvm.org/docs/CodeGenerator.html

祝学习愉快！🎉
