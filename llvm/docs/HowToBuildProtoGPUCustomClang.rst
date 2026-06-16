How To Build ProtoGPU Custom Clang (NVPTX + warp_reduce_add)
============================================================

This guide records a practical build flow for a custom LLVM/Clang toolchain
that can emit PTX ``warp_reduce_add.f32`` from a custom NVVM builtin/intrinsic
path.

Prerequisites
-------------

1. LLVM monorepo checkout with your custom changes.
2. CMake and Ninja installed.
3. A CUDA Toolkit installation (for compiling CUDA sources in downstream use).

Configure And Build
-------------------

Use a dedicated build directory and enable ``NVPTX`` in
``LLVM_TARGETS_TO_BUILD``.

.. code-block:: bash

   cd /path/to/llvm-project
   cmake -S llvm -B build-nvptx -G Ninja \
     -DCMAKE_BUILD_TYPE=Release \
     -DLLVM_ENABLE_PROJECTS=clang \
     -DLLVM_TARGETS_TO_BUILD="X86;NVPTX"

   ninja -C build-nvptx clang llc

Parallel Build (``-j``) Caveat
------------------------------

Large LLVM/Clang builds can fail intermittently on memory-constrained machines
when ``-j`` is set too high (for example, random ``cc1plus`` termination,
``Killed`` by OOM, or link steps failing under pressure).

Recommended practice:

.. code-block:: bash

   # Start conservative, then increase if stable.
   ninja -C build-nvptx -j4 clang llc

If you hit OOM-like failures, reduce parallelism (for example ``-j2``) and
retry.

Quick Verification
------------------

Verify the backend is present:

.. code-block:: bash

   build-nvptx/bin/llc --version

Expected: registered targets include ``nvptx`` and ``nvptx64``.

Verify custom intrinsic to PTX opcode lowering:

.. code-block:: bash

   build-nvptx/bin/llc llvm/test/CodeGen/NVPTX/warp-reduce-add.ll \
     -mtriple=nvptx64 -mcpu=sm_70 -o - | grep warp_reduce_add.f32

Expected output contains ``warp_reduce_add.f32``.

Run focused LLVM/Clang tests:

.. code-block:: bash

   ninja -C build-nvptx check-llvm \
     LLVM_LIT_ARGS="llvm/test/CodeGen/NVPTX/warp-reduce-add.ll -v"

   ninja -C build-nvptx check-clang \
     LLVM_LIT_ARGS="clang/test/CodeGenCUDA/warp-reduce-add-builtin.cu -v"

Using The Custom Clang In ProtoGPU
----------------------------------

ProtoGPU scripts honor ``CLANGXX``. Point it to your custom build:

.. code-block:: bash

   export CLANGXX=/path/to/llvm-project/build-nvptx/bin/clang++

Then run the existing PTX generation/integration scripts normally.

Notes
-----

1. ``__nvvm_warp_reduce_add`` is a compiler builtin / NVVM intrinsic entry
   point, not a PTX instruction mnemonic.
2. The PTX instruction mnemonic emitted by the backend is
   ``warp_reduce_add.f32``.
3. NVIDIA ``ptxas`` may reject custom opcodes; this is expected for custom ISA
   extensions intended for ProtoGPU consumption.
4. Known failure mode: even with this custom clang, building a ``.cu``
   executable that includes ``warp_reduce_add.f32`` in device code can still
   fail. PTX text generation succeeds, but executable builds typically require
   downstream CUDA assembler/JIT stages that do not recognize custom opcodes.
   Use the split flow (host executable source + PTX override source) for
   end-to-end ProtoGPU runs.
5. Register naming may differ between paths: inline PTX asm examples often show
   ``%f`` registers, while backend-lowered custom clang output may use ``%r``
   (``.b32``) temporaries around ``warp_reduce_add.f32``. This difference is
   expected and does not by itself indicate a semantic mismatch; the opcode
   suffix (``.f32``) defines floating-point interpretation.
