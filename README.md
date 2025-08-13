# self-hosted-compiler

A minimal self-hosted compiler for a subset of C, targeting macOS arm64 (AArch64), for learning.

- Uses only POSIX APIs (open/read/write/close, malloc/free, etc.).
- Emits Apple/Clang-compatible ARM64 assembly.
- One-pass Pratt parser with on-the-fly codegen (stack-temporary style).
