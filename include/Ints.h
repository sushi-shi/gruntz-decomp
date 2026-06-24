// Ints.h - Rust-style fixed-width integer aliases (like the vostok engine).
//
// Widths are this target's: 32-bit Win32 / MSVC 5.0, where `long` == `int` == 4
// bytes and `unsigned long` == `unsigned int` == 4 bytes. So `i32`/`u32` cover
// BOTH `int`/`long` and `unsigned`/`unsigned long` here; this keeps codegen
// byte-identical (widths preserved) while making the shapes read clearly and a
// future 64-bit port easier. `i32` IS `int`, so `int`->`i32` is a no-op for both
// codegen and name-mangling; `long`->`i32` changes only the mangled name (the
// RVA-keyed pairing + reloc-masking absorb that).
//
// NOTE: the SDK's own aliases (BOOL/DWORD/WORD/BYTE/UINT/INT/LONG/...) are left
// as-is in our sources - they pin our externs to the real Win32/MFC/DX
// signatures. These aliases are ONLY for our raw int/long/unsigned/short/
// signed|unsigned char. Plain `char` (text) stays `char`.
#ifndef GRUNTZ_INTS_H
#define GRUNTZ_INTS_H

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;

#endif // GRUNTZ_INTS_H
