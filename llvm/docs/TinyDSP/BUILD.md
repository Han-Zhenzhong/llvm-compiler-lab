# TinyDSP Backend - Build and Integration Instructions

## 构建说明

### 系统要求

- **操作系统**: Windows 10+, Linux (Ubuntu 20.04+), macOS 11+
- **CMake**: 3.20 或更高版本
- **构建工具**: Ninja (推荐) 或 Make
- **编译器**: 
  - GCC 7+ 或 Clang 5+ (Linux/macOS)
  - MSVC 2019+ 或 Clang-cl (Windows)
- **Python**: 3.7+ (用于运行模拟器和测试)
- **磁盘空间**: 至少 20GB (用于 LLVM 构建)
- **内存**: 建议 16GB+

### 构建步骤

#### 步骤 1: 准备 LLVM 源码

如果还没有 LLVM 源码:

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
```

TinyDSP 后端已经位于 `llvm/lib/Target/TinyDSP/` 目录。

#### 步骤 2: 创建构建目录

```bash
mkdir build
cd build
```

#### 步骤 3: 配置 CMake

**Windows (使用 Ninja):**
```cmd
cmake -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" ^
  -DLLVM_ENABLE_PROJECTS="clang" ^
  -DLLVM_ENABLE_ASSERTIONS=ON ^
  -DCMAKE_INSTALL_PREFIX=../install ^
  ..\llvm
```

**Linux/macOS:**
```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_INSTALL_PREFIX=../install \
  ../llvm
```

**配置说明**:
- `-DLLVM_TARGETS_TO_BUILD="X86;TinyDSP"` - 同时构建 X86 和 TinyDSP 目标
- `-DLLVM_ENABLE_PROJECTS="clang"` - 启用 Clang 前端
- `-DLLVM_ENABLE_ASSERTIONS=ON` - 启用断言（用于调试）
- `-DCMAKE_INSTALL_PREFIX` - 安装路径

#### 步骤 4: 编译

```bash
# 完整构建 (包括 Clang 和所有目标)
ninja

# 或者只构建 TinyDSP 相关组件
ninja TinyDSPCodeGen TinyDSPDesc TinyDSPInfo clang

# 可选: 安装到指定目录
ninja install
```

**构建时间**: 
- 首次完整构建: 30-60 分钟 (取决于硬件)
- 增量构建: 1-5 分钟

### 验证构建

#### 1. 检查 TinyDSP 目标是否注册

```bash
# Windows
build\bin\llc --version | findstr TinyDSP

# Linux/macOS
./build/bin/llc --version | grep TinyDSP
```

应该看到输出:
```
TinyDSP
```

#### 2. 列出所有支持的目标

```bash
./build/bin/llc -version
```

应该在目标列表中看到 `tinydsp`。

#### 3. 测试编译

创建测试文件 `test.c`:
```c
int add(int a, int b) {
    return a + b;
}
```

编译:
```bash
./build/bin/clang -target tinydsp -S test.c -o test.s
cat test.s
```

应该看到 TinyDSP 汇编输出。

### 常见构建问题

#### 问题 1: CMake 找不到 TinyDSP

**症状**: `llc --version` 中没有 TinyDSP

**解决**:
```bash
# 清理构建目录
rm -rf build/*

# 重新配置，确保包含 TinyDSP
cmake -G Ninja -DLLVM_TARGETS_TO_BUILD="X86;TinyDSP" ../llvm

# 重新构建
ninja
```

#### 问题 2: TableGen 错误

**症状**: 编译时出现 TableGen 相关错误

**解决**:
```bash
# 检查 TableGen 文件语法
./build/bin/llvm-tblgen \
  -I llvm/include \
  -I llvm/lib/Target/TinyDSP \
  llvm/lib/Target/TinyDSP/TinyDSP.td
```

如果有语法错误，会显示具体的错误位置。

#### 问题 3: 链接错误

**症状**: 链接时找不到 TinyDSP 符号

**解决**:
```bash
# 确保所有 TinyDSP 组件都已构建
ninja TinyDSPCodeGen TinyDSPDesc TinyDSPInfo

# 检查生成的库文件
ls build/lib/libLLVMTinyDSP*
```

#### 问题 4: Python 测试失败

**症状**: `verify_tinydsp.py` 运行失败

**解决**:
```bash
# 检查 Python 版本
python --version  # 应该 >= 3.7

# 检查 clang 路径
which ./build/bin/clang

# 手动测试模拟器
cd llvm/utils/TinyDSP
python tinydsp_simulator.py
```

### 增量构建

修改 TinyDSP 代码后:

```bash
cd build

# 只重新构建 TinyDSP
ninja TinyDSPCodeGen

# 或者完整重新构建
ninja
```

### 调试构建

如果需要调试 TinyDSP 后端:

```bash
# 配置 Debug 构建
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_TARGETS_TO_BUILD="TinyDSP" \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  ../llvm

ninja

# 使用 gdb/lldb 调试
gdb --args ./bin/clang -target tinydsp -S test.c
```

### 性能优化构建

对于生产使用:

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="TinyDSP" \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  ../llvm

ninja
```

## 集成到现有 LLVM 安装

如果已经有 LLVM 安装，只想添加 TinyDSP:

### 方法 1: 重新构建 LLVM (推荐)

```bash
cd llvm-project/build
cmake -DLLVM_TARGETS_TO_BUILD="...;TinyDSP" ../llvm
ninja
```

### 方法 2: 构建为插件 (实验性)

TinyDSP 当前不支持作为插件加载，需要完整重新构建 LLVM。

## 测试套件

### 运行所有测试

```bash
cd llvm/lib/Target/TinyDSP

# 模拟器自测
python llvm/utils/TinyDSP/tinydsp_simulator.py

# 完整验证测试
python llvm/utils/TinyDSP/verify_tinydsp.py ../../build

# 运行 LLVM 回归测试 (如果有)
cd ../../build
ninja check-llvm-codegen-tinydsp
```

### 添加自定义测试

在 `test/` 目录创建新的 C 文件:

```c
// test/my_test.c
int my_function(int x) {
    return x * 2 + 1;
}
```

使用验证脚本测试:

```python
# 在 verify_tinydsp.py 中添加:
tests = [
    # ...
    ("my_function", [5], 11),  # 5*2+1 = 11
]
```

## 卸载

删除构建产物:

```bash
# 删除构建目录
rm -rf build

# 删除安装目录 (如果使用了 ninja install)
rm -rf install
```

## 下一步

成功构建后:

1. **阅读快速开始指南**: `QUICKSTART.md`
2. **查看使用示例**: `README.md`
3. **运行验证测试**: `llvm/utils/TinyDSP/verify_tinydsp.py`
4. **尝试修改后端**: 添加新指令或优化

## 获取帮助

构建问题？

1. 检查 LLVM 官方文档: https://llvm.org/docs/GettingStarted.html
2. 查看 TinyDSP README: `README.md`
3. 查看 LLVM 构建系统文档: https://llvm.org/docs/CMake.html

祝构建成功！🎉
