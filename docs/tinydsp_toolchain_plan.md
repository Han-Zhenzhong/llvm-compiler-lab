# TinyDSP: From Assembly to Binary Executable

**Development Plan for Complete Toolchain Implementation**

---

## 目标 (Objective)

扩展 TinyDSP LLVM 后端，实现从汇编代码到可执行二进制文件的完整工具链，包括：
- 汇编器 (Assembler): `.s` → `.o`
- 链接器 (Linker): 多个 `.o` → 可执行文件
- 运行时支持库 (Runtime Library)
- 加载器/模拟器 (Loader/Simulator)

---

## 当前状态 (Current Status)

✅ **已完成**:
- C 代码 → LLVM IR → TinyDSP 汇编 (`.s`)
- MCCodeEmitter: 指令编码逻辑
- ELF 对象文件框架 (TinyDSPELFObjectWriter)
- 基础 AsmBackend

❌ **待实现**:
- 完整的汇编器 (Assembler)
- 重定位支持 (Relocations)
- 链接器 (Linker)
- C 运行时库 (CRT)
- 可执行文件加载和执行

---

## 实施阶段 (Implementation Phases)

### 阶段 1: 完善汇编器支持 (Assembler Enhancement)

**目标**: 支持从 `.s` 汇编文件生成 `.o` 对象文件

#### 1.1 实现 AsmParser

**文件**: `llvm/lib/Target/TinyDSP/AsmParser/TinyDSPAsmParser.cpp`

```cpp
// 需要实现的功能:
- 解析汇编指令 (ADD, SUB, LOAD, etc.)
- 解析寄存器名称 (r0-r7)
- 解析立即数和标签
- 生成 MCInst 对象
- 支持伪指令展开
```

**关键类**:
```cpp
class TinyDSPAsmParser : public MCTargetAsmParser {
  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo, bool MatchingInlineAsm) override;
};
```

**测试命令**:
```bash
llvm-mc -arch=tinydsp -filetype=obj input.s -o output.o
```

#### 1.2 增强重定位支持

**文件**: `llvm/lib/Target/TinyDSP/MCTargetDesc/TinyDSPFixupKinds.h`

定义重定位类型:
```cpp
enum Fixups {
  // 32-bit absolute address
  fixup_tinydsp_abs32 = FirstTargetFixupKind,
  
  // PC-relative 18-bit for branches
  fixup_tinydsp_pcrel18,
  
  // High/Low 16-bit for address loading
  fixup_tinydsp_hi16,
  fixup_tinydsp_lo16,
  
  // Last marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
```

**文件**: `llvm/lib/Target/TinyDSP/MCTargetDesc/TinyDSPELFObjectWriter.cpp`

实现 `getRelocType()`:
```cpp
unsigned TinyDSPELFObjectWriter::getRelocType(MCContext &Ctx,
                                               const MCValue &Target,
                                               const MCFixup &Fixup,
                                               bool IsPCRel) const {
  unsigned Type;
  unsigned Kind = Fixup.getKind();
  
  switch (Kind) {
  case FK_Data_4:
  case TinyDSP::fixup_tinydsp_abs32:
    Type = ELF::R_TINYDSP_32;
    break;
  case TinyDSP::fixup_tinydsp_pcrel18:
    Type = ELF::R_TINYDSP_PCREL18;
    break;
  // ... more cases
  }
  return Type;
}
```

#### 1.3 实现 Fixup 应用

**文件**: `llvm/lib/Target/TinyDSP/MCTargetDesc/TinyDSPAsmBackend.cpp`

```cpp
void TinyDSPAsmBackend::applyFixup(const MCAssembler &Asm,
                                   const MCFixup &Fixup,
                                   const MCValue &Target,
                                   MutableArrayRef<char> Data,
                                   uint64_t Value,
                                   bool IsResolved,
                                   const MCSubtargetInfo *STI) const {
  unsigned Kind = Fixup.getKind();
  unsigned Offset = Fixup.getOffset();
  
  switch (Kind) {
  case FK_Data_4:
    // 32-bit absolute
    support::endian::write<uint32_t>(
        Data.data() + Offset, Value, endianness::little);
    break;
    
  case TinyDSP::fixup_tinydsp_pcrel18:
    // 18-bit PC-relative
    assert(isInt<18>(Value) && "Out of range PC-relative fixup");
    uint32_t Instr = support::endian::read<uint32_t>(
        Data.data() + Offset, endianness::little);
    Instr = (Instr & ~0x3FFFF) | (Value & 0x3FFFF);
    support::endian::write<uint32_t>(
        Data.data() + Offset, Instr, endianness::little);
    break;
  }
}
```

**验证**:
```bash
# 编译带有外部符号引用的代码
cat > test_reloc.s << EOF
    .global main
main:
    li r1, external_func
    ret
EOF

llvm-mc -arch=tinydsp -filetype=obj test_reloc.s -o test_reloc.o
llvm-readobj --relocs test_reloc.o
# 应该看到重定位条目
```

---

### 阶段 2: 实现 ELF 规范 (ELF Specification)

**目标**: 定义 TinyDSP 特定的 ELF 格式

#### 2.1 定义 ELF 机器类型

**文件**: `llvm/include/llvm/BinaryFormat/ELF.h`

添加 TinyDSP 定义:
```cpp
// 在 EM_* 枚举中添加
EM_TINYDSP = 0x9999,  // 临时编号，正式应向 ELF spec 申请

// TinyDSP 重定位类型
enum {
  R_TINYDSP_NONE = 0,
  R_TINYDSP_32 = 1,
  R_TINYDSP_PCREL18 = 2,
  R_TINYDSP_HI16 = 3,
  R_TINYDSP_LO16 = 4,
  R_TINYDSP_CALL = 5,
};
```

#### 2.2 更新 ELF Writer

**文件**: `llvm/lib/Target/TinyDSP/MCTargetDesc/TinyDSPELFObjectWriter.cpp`

```cpp
TinyDSPELFObjectWriter::TinyDSPELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit*/ false, OSABI, 
                              ELF::EM_TINYDSP,  // 使用新定义的机器类型
                              /*HasRelocationAddend*/ false) {}
```

**验证**:
```bash
llvm-readelf -h output.o | grep Machine
# 应该显示: Machine: TinyDSP
```

---

### 阶段 3: 实现链接器支持 (Linker Support)

**目标**: 将多个 `.o` 文件链接成可执行文件

#### 3.1 选项 A: 使用 LLD (推荐)

**文件**: `lld/ELF/Arch/TinyDSP.cpp`

```cpp
namespace {
class TinyDSP final : public TargetInfo {
public:
  TinyDSP();
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
}

TinyDSP::TinyDSP() {
  // 32-bit little-endian
  defaultMaxPageSize = 4096;
  defaultImageBase = 0x10000;
}

RelExpr TinyDSP::getRelExpr(RelType type, const Symbol &s,
                             const uint8_t *loc) const {
  switch (type) {
  case R_TINYDSP_32:
    return R_ABS;
  case R_TINYDSP_PCREL18:
    return R_PC;
  case R_TINYDSP_CALL:
    return R_PLT_PC;
  default:
    error(getErrorLocation(loc) + "unknown relocation (" + 
          Twine(type) + ") against symbol " + toString(s));
    return R_NONE;
  }
}

void TinyDSP::relocate(uint8_t *loc, const Relocation &rel,
                       uint64_t val) const {
  switch (rel.type) {
  case R_TINYDSP_32:
    write32le(loc, val);
    break;
  case R_TINYDSP_PCREL18: {
    uint32_t instr = read32le(loc);
    instr = (instr & ~0x3FFFF) | ((val - rel.offset) & 0x3FFFF);
    write32le(loc, instr);
    break;
  }
  default:
    error(getErrorLocation(loc) + "unrecognized relocation " + 
          toString(rel.type));
  }
}
```

**注册链接器**:

**文件**: `lld/ELF/Driver.cpp`

```cpp
// 在 link() 函数中添加:
case EM_TINYDSP:
  return elf::link<ELF32LE>(args, /*canExitEarly=*/true);
```

**测试链接**:
```bash
# 编译多个文件
clang -target tinydsp -c main.c -o main.o
clang -target tinydsp -c lib.c -o lib.o

# 链接
ld.lld -m tinydsp main.o lib.o -o program

# 查看结果
llvm-readelf -a program
```

#### 3.2 选项 B: 简化链接器 (教学用)

**文件**: `llvm/lib/Target/TinyDSP/tools/tinydsp-ld.py`

```python
#!/usr/bin/env python3
"""
简化的 TinyDSP 链接器
仅用于教学目的，支持基本的静态链接
"""

import sys
from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

class TinyDSPLinker:
    def __init__(self):
        self.objects = []
        self.symbols = {}  # name -> (object_idx, section, value)
        self.sections = []
        self.base_address = 0x10000
        
    def add_object(self, obj_file):
        """添加一个对象文件"""
        with open(obj_file, 'rb') as f:
            elf = ELFFile(f)
            self.objects.append(elf)
            self._extract_symbols(elf, len(self.objects) - 1)
    
    def _extract_symbols(self, elf, obj_idx):
        """提取符号表"""
        symtab = elf.get_section_by_name('.symtab')
        if not symtab:
            return
            
        for symbol in symtab.iter_symbols():
            if symbol['st_info']['bind'] == 'STB_GLOBAL':
                self.symbols[symbol.name] = {
                    'object': obj_idx,
                    'section': symbol['st_shndx'],
                    'value': symbol['st_value'],
                    'size': symbol['st_size']
                }
    
    def resolve_relocations(self):
        """解析重定位"""
        for obj_idx, elf in enumerate(self.objects):
            for section in elf.iter_sections():
                if isinstance(section, RelocationSection):
                    self._apply_relocations(section, obj_idx)
    
    def _apply_relocations(self, relsec, obj_idx):
        """应用重定位到目标段"""
        target_section = self.objects[obj_idx].get_section(relsec['sh_info'])
        data = bytearray(target_section.data())
        
        for rel in relsec.iter_relocations():
            symbol = rel['r_info_sym']
            rel_type = rel['r_info_type']
            offset = rel['r_offset']
            
            # 查找符号地址
            if symbol in self.symbols:
                target_addr = self._get_symbol_address(symbol)
            else:
                raise ValueError(f"Undefined symbol: {symbol}")
            
            # 应用重定位
            if rel_type == 1:  # R_TINYDSP_32
                data[offset:offset+4] = target_addr.to_bytes(4, 'little')
            # ... 处理其他重定位类型
        
        return data
    
    def link(self, output_file):
        """执行链接，生成可执行文件"""
        # 1. 收集所有段
        # 2. 分配地址
        # 3. 解析重定位
        # 4. 生成可执行 ELF
        pass

# 使用示例
if __name__ == "__main__":
    linker = TinyDSPLinker()
    
    for obj_file in sys.argv[1:-1]:
        linker.add_object(obj_file)
    
    output = sys.argv[-1]
    linker.link(output)
```

---

### 阶段 4: C 运行时库 (C Runtime Library)

**目标**: 提供基本的 C 运行时支持

#### 4.1 启动代码 (Startup Code)

**文件**: `llvm/lib/Target/TinyDSP/runtime/crt0.s`

```asm
    .global _start
    .type _start, @function

_start:
    # 初始化栈指针
    li r7, 0x10000      # 栈顶地址
    
    # 清零 BSS 段
    li r1, __bss_start
    li r2, __bss_end
.Lclear_bss:
    store r0, [r1 + 0]
    addi r1, r1, 4
    sub r3, r2, r1
    # 如果 r3 > 0 继续循环 (需要条件分支指令)
    
    # 调用全局构造函数
    # call __libc_init_array
    
    # 调用 main 函数
    li r2, 0            # argc = 0
    li r3, 0            # argv = NULL
    # call main
    
    # 退出程序
    # call exit

.Lexit_loop:
    # 无限循环 (等待外部中断)
    # j .Lexit_loop
```

#### 4.2 基础 C 库函数

**文件**: `llvm/lib/Target/TinyDSP/runtime/tinydsp_runtime.c`

```c
// 基础内存操作
void *memset(void *s, int c, unsigned long n) {
    unsigned char *p = s;
    while (n--)
        *p++ = c;
    return s;
}

void *memcpy(void *dest, const void *src, unsigned long n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

// 系统调用接口 (假设通过特殊指令或 MMIO)
void __tinydsp_syscall(int num, int arg1, int arg2, int arg3) {
    // 实现取决于目标平台
    // 可能是 MMIO 写入、trap 指令等
    volatile int *syscall_reg = (int*)0xFFFF0000;
    syscall_reg[0] = num;
    syscall_reg[1] = arg1;
    syscall_reg[2] = arg2;
    syscall_reg[3] = arg3;
}

void exit(int status) {
    __tinydsp_syscall(1, status, 0, 0);  // SYS_exit
    while(1);  // 永不返回
}

// 简单的 printf (仅支持 %d, %s)
int printf(const char *format, ...) {
    // 简化实现：输出到调试端口
    // 完整实现需要 va_list 支持
    return 0;
}
```

#### 4.3 构建运行时库

**文件**: `llvm/lib/Target/TinyDSP/runtime/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(TinyDSPRuntime C ASM)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-target tinydsp -nostdlib -ffreestanding")

add_library(tinydsp_rt STATIC
    crt0.s
    tinydsp_runtime.c
)

install(TARGETS tinydsp_rt
    ARCHIVE DESTINATION lib/tinydsp)
```

**使用运行时库**:
```bash
clang -target tinydsp -c main.c -o main.o
ld.lld -m tinydsp \
    crt0.o \
    main.o \
    -ltinydsp_rt \
    -o program
```

---

### 阶段 5: 可执行文件加载与执行 (Loader & Execution)

**目标**: 在模拟器中加载和执行 ELF 可执行文件

#### 5.1 增强模拟器支持 ELF 加载

**文件**: `llvm/lib/Target/TinyDSP/tools/tinydsp_simulator.py`

添加 ELF 加载器:
```python
from elftools.elf.elffile import ELFFile
from elftools.elf.segments import Segment

class TinyDSPSimulator:
    # ... 现有代码 ...
    
    def load_elf(self, elf_file):
        """加载 ELF 可执行文件"""
        with open(elf_file, 'rb') as f:
            elf = ELFFile(f)
            
            # 检查架构
            if elf['e_machine'] != 'EM_TINYDSP':
                raise ValueError("Not a TinyDSP executable")
            
            # 加载程序段到内存
            for segment in elf.iter_segments():
                if segment['p_type'] == 'PT_LOAD':
                    self._load_segment(segment)
            
            # 设置入口点
            self.pc = elf['e_entry']
            
            # 初始化栈
            self.regs[7] = 0x10000  # SP
    
    def _load_segment(self, segment):
        """加载单个段到内存"""
        vaddr = segment['p_vaddr']
        data = segment.data()
        
        # 复制数据到内存
        for i, byte in enumerate(data):
            self.memory[vaddr + i] = byte
    
    def run_elf(self, elf_file, max_cycles=10000):
        """加载并执行 ELF 文件"""
        self.load_elf(elf_file)
        
        cycle = 0
        while cycle < max_cycles:
            # 从内存读取指令
            if self.pc >= len(self.memory) - 4:
                print(f"PC out of bounds: 0x{self.pc:x}")
                break
            
            instr_word = self.read_word(self.pc)
            
            # 解码并执行指令
            if not self.execute_encoded_instruction(instr_word):
                break  # RET 或错误
            
            self.pc += 4
            cycle += 1
        
        print(f"Execution completed in {cycle} cycles")
        return self.regs[0]  # 返回值
    
    def execute_encoded_instruction(self, instr_word):
        """执行编码的机器指令"""
        opcode = (instr_word >> 24) & 0xFF
        
        if opcode == 0x01:  # ADD
            rd = (instr_word >> 21) & 0x7
            rs1 = (instr_word >> 18) & 0x7
            rs2 = (instr_word >> 15) & 0x7
            self.regs[rd] = (self.regs[rs1] + self.regs[rs2]) & 0xFFFFFFFF
            
        elif opcode == 0x30:  # RET
            return False
            
        # ... 实现其他指令 ...
        
        return True
```

#### 5.2 系统调用模拟

```python
class TinyDSPSimulator:
    # ... 现有代码 ...
    
    SYSCALL_EXIT = 1
    SYSCALL_WRITE = 4
    
    def handle_syscall(self):
        """处理系统调用"""
        syscall_num = self.regs[0]
        
        if syscall_num == self.SYSCALL_EXIT:
            exit_code = self.regs[1]
            print(f"Program exited with code {exit_code}")
            return False  # 停止执行
            
        elif syscall_num == self.SYSCALL_WRITE:
            fd = self.regs[1]
            buf_addr = self.regs[2]
            count = self.regs[3]
            
            if fd == 1:  # stdout
                data = bytes(self.memory[buf_addr:buf_addr+count])
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
            
            self.regs[0] = count  # 返回写入的字节数
            
        return True
```

#### 5.3 完整的测试流程

**文件**: `llvm/lib/Target/TinyDSP/test/test_complete.c`

```c
// 完整的 C 程序测试
extern void exit(int status);

int factorial(int n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

int main(int argc, char *argv[]) {
    int result = factorial(5);
    
    // result 应该是 120
    if (result == 120)
        exit(0);  // 成功
    else
        exit(1);  // 失败
}
```

**构建和运行脚本**: `test/build_and_run.sh`

```bash
#!/bin/bash

# 1. 编译 C 代码到汇编
clang -target tinydsp -S test_complete.c -o test_complete.s

# 2. 汇编为对象文件
llvm-mc -arch=tinydsp -filetype=obj test_complete.s -o test_complete.o
llvm-mc -arch=tinydsp -filetype=obj ../runtime/crt0.s -o crt0.o

# 3. 编译运行时库
clang -target tinydsp -c ../runtime/tinydsp_runtime.c -o runtime.o

# 4. 链接
ld.lld -m tinydsp \
    crt0.o \
    test_complete.o \
    runtime.o \
    -o test_complete.elf

# 5. 查看可执行文件信息
llvm-readelf -a test_complete.elf

# 6. 在模拟器中运行
python ../tools/tinydsp_simulator.py test_complete.elf

# 7. 检查退出代码
if [ $? -eq 0 ]; then
    echo "✅ Test passed!"
else
    echo "❌ Test failed!"
fi
```

---

### 阶段 6: 调试支持 (Debug Support)

**目标**: 支持 DWARF 调试信息和 GDB 调试

#### 6.1 生成调试信息

确保编译时生成 DWARF:
```bash
clang -target tinydsp -g -c test.c -o test.o
```

#### 6.2 GDB 支持 (可选)

**文件**: `gdb/gdb/tinydsp-tdep.c`

```c
// GDB target description for TinyDSP
static struct gdbarch *
tinydsp_gdbarch_init(struct gdbarch_info info, struct gdbarch_list *arches)
{
    struct gdbarch *gdbarch;
    
    // 创建新架构
    gdbarch = gdbarch_alloc(&info, NULL);
    
    // 设置寄存器
    set_gdbarch_num_regs(gdbarch, 8);
    set_gdbarch_pc_regnum(gdbarch, -1);
    set_gdbarch_sp_regnum(gdbarch, 7);
    
    // 设置字节序和字长
    set_gdbarch_byte_order(gdbarch, BFD_ENDIAN_LITTLE);
    set_gdbarch_ptr_bit(gdbarch, 32);
    set_gdbarch_addr_bit(gdbarch, 32);
    
    return gdbarch;
}
```

#### 6.3 模拟器 GDB 服务器

**文件**: `llvm/lib/Target/TinyDSP/tools/tinydsp_gdbserver.py`

```python
import socket
import struct

class TinyDSPGDBServer:
    """GDB 远程调试协议服务器"""
    
    def __init__(self, simulator, port=1234):
        self.sim = simulator
        self.port = port
        self.sock = None
        
    def start(self):
        """启动 GDB 服务器"""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('localhost', self.port))
        self.sock.listen(1)
        
        print(f"GDB server listening on port {self.port}")
        conn, addr = self.sock.accept()
        print(f"GDB connected from {addr}")
        
        self.handle_gdb_commands(conn)
    
    def handle_gdb_commands(self, conn):
        """处理 GDB 命令"""
        while True:
            cmd = self.receive_packet(conn)
            if not cmd:
                break
            
            response = self.process_command(cmd)
            self.send_packet(conn, response)
    
    def process_command(self, cmd):
        """处理单个 GDB 命令"""
        if cmd == 'g':  # 读取所有寄存器
            regs = ''.join(f'{r:08x}' for r in self.sim.regs)
            return regs
            
        elif cmd.startswith('m'):  # 读取内存
            addr, length = cmd[1:].split(',')
            addr = int(addr, 16)
            length = int(length, 16)
            data = self.sim.memory[addr:addr+length]
            return data.hex()
            
        elif cmd == 's':  # 单步执行
            self.sim.step()
            return 'S05'  # SIGTRAP
            
        elif cmd == 'c':  # 继续执行
            self.sim.run()
            return 'S00'
        
        return ''
```

---

## 实施时间表 (Timeline)

| 阶段 | 任务 | 预计时间 | 优先级 |
|------|------|----------|--------|
| 1 | 汇编器支持 | 1-2 周 | 高 |
| 2 | ELF 规范 | 3-5 天 | 高 |
| 3 | 链接器 (LLD) | 2-3 周 | 高 |
| 3 | 链接器 (简化) | 1 周 | 中 |
| 4 | C 运行时库 | 1-2 周 | 高 |
| 5 | ELF 加载器 | 1 周 | 高 |
| 5 | 系统调用模拟 | 3-5 天 | 中 |
| 6 | 调试支持 | 2-3 周 | 低 |

**总计**: 约 2-3 个月全职开发时间

---

## 测试策略 (Testing Strategy)

### 单元测试
```bash
# 测试汇编器
llvm-lit llvm/test/MC/TinyDSP/

# 测试重定位
llvm-lit llvm/test/CodeGen/TinyDSP/reloc-*.ll

# 测试链接器
llvm-lit lld/test/ELF/tinydsp-*.s
```

### 集成测试
```python
# test/integration_test.py
def test_hello_world():
    """测试 Hello World 程序的完整流程"""
    # 编译
    compile("hello.c", "hello.o")
    
    # 链接
    link(["crt0.o", "hello.o", "libc.a"], "hello.elf")
    
    # 运行
    output = run_simulator("hello.elf")
    assert output == "Hello, TinyDSP!\n"
```

### 回归测试
```bash
# 运行完整的测试套件
./test/run_all_tests.sh

# 输出:
# ✓ asm_parser: 45/45 tests passed
# ✓ code_gen: 30/30 tests passed
# ✓ linker: 20/20 tests passed
# ✓ runtime: 15/15 tests passed
# ✓ simulator: 25/25 tests passed
# 
# Total: 135/135 tests passed (100%)
```

---

## 参考资料 (References)

### LLVM 文档
- [LLVM MC Layer](https://llvm.org/docs/CodeGenerator.html#the-mc-layer)
- [TableGen AsmParser](https://llvm.org/docs/TableGen/BackEnds.html#asm-parser)
- [Relocation Models](https://llvm.org/docs/LangRef.html#runtime-preemption-specifiers)

### ELF 规范
- [ELF Specification](https://refspecs.linuxfoundation.org/elf/elf.pdf)
- [Generic ABI](https://www.sco.com/developers/gabi/)

### 链接器设计
- [LLD Documentation](https://lld.llvm.org/)
- [Linkers and Loaders](https://www.iecc.com/linker/)

### 类似项目参考
- **RISC-V**: `llvm/lib/Target/RISCV/`, `lld/ELF/Arch/RISCV.cpp`
- **Lanai**: `llvm/lib/Target/Lanai/` (简单的 32-bit 架构)
- **MSP430**: `llvm/lib/Target/MSP430/` (小型嵌入式)

---

## 成功标准 (Success Criteria)

完成以下验收测试，即可认为工具链实现成功：

### ✅ 基础功能
- [ ] 单个 C 文件编译、汇编、链接、运行成功
- [ ] 多文件项目链接成功
- [ ] 支持全局变量和函数调用
- [ ] 支持静态链接库

### ✅ 运行时支持
- [ ] C 标准库基础函数可用 (memset, memcpy, etc.)
- [ ] 程序能正确初始化和退出
- [ ] 系统调用接口工作正常

### ✅ 调试支持
- [ ] 生成正确的 DWARF 调试信息
- [ ] 模拟器支持断点和单步执行
- [ ] 能查看寄存器和内存状态

### ✅ 完整示例
能够成功编译并运行以下程序:

```c
// fibonacci.c
#include <stdio.h>

int fib(int n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}

int main() {
    for (int i = 0; i < 10; i++) {
        printf("fib(%d) = %d\n", i, fib(i));
    }
    return 0;
}
```

输出:
```
fib(0) = 0
fib(1) = 1
fib(2) = 1
fib(3) = 2
...
fib(9) = 34
```

---

## 下一步扩展方向 (Future Enhancements)

完成基础工具链后，可以考虑以下扩展：

1. **性能优化**
   - Link-Time Optimization (LTO)
   - Profile-Guided Optimization (PGO)
   - 指令调度优化

2. **高级特性**
   - 动态链接支持 (.so)
   - Position Independent Code (PIC)
   - Thread-Local Storage (TLS)

3. **硬件集成**
   - 与 FPGA 开发流程集成
   - Verilator 协同仿真
   - 硬件加速器接口

4. **生态系统**
   - CMake toolchain 文件
   - 包管理器集成
   - IDE 支持 (VS Code, CLion)

---

**文档版本**: 1.0  
**创建日期**: 2025-11-29  
**维护者**: TinyDSP Project Team
