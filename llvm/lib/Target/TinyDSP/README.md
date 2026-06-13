# TinyDSP Backend Layout

This directory contains only TinyDSP backend source code and TableGen definitions,
following LLVM target backend conventions.

## Source Layout

- Core backend sources: `llvm/lib/Target/TinyDSP/*.cpp`, `*.h`, `*.td`
- MC layer: `llvm/lib/Target/TinyDSP/MCTargetDesc/`
- Target registration: `llvm/lib/Target/TinyDSP/TargetInfo/`

## TinyDSP Documentation

- `llvm/docs/TinyDSP/README.md`
- `llvm/docs/TinyDSP/QUICKSTART.md`
- `llvm/docs/TinyDSP/BUILD.md`
- `llvm/docs/TinyDSP/IMPLEMENTATION_SUMMARY.md`

## TinyDSP Utilities and Test Inputs

- Utility scripts: `llvm/utils/TinyDSP/`
- TinyDSP test inputs used by helper scripts: `llvm/test/CodeGen/TinyDSP/Inputs/`
