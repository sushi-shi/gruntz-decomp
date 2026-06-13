# zlib 1.0.4 — vendored matching source

## What / where
`vendor/zlib-1.0.4/` — the upstream zlib **1.0.4** source, vendored into the repo
(zlib license, so safe to commit).

- Source: `https://zlib.net/fossils/zlib-1.0.4.tar.gz`
- tarball sha256: `e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24`
- Confirmed `#define ZLIB_VERSION "1.0.4"` in `zlib.h` — exactly the build statically
  linked into `GRUNTZ.EXE` (the binary carries `deflate 1.0.4 Copyright 1995-1996
  Jean-loup Gailly` / `inflate 1.0.4 Copyright 1995-1996 Mark Adler`).

## Why zlib is the first matching target
zlib is statically linked into `GRUNTZ.EXE` (it's the WWD/REZ decompressor — `.WWD`
levels are deflate-compressed). It's the **cleanest calibration target**: well-known
public source, small, and exactly versioned. Compiling it with our VC++ 5.0 SP3
toolchain and iterating compiler flags until the emitted objects **byte-match** the
zlib functions in `GRUNTZ.EXE` pins down the **exact flag set** (e.g. `/MT`, opt level
`/O1` vs `/O2`, `/GF`, struct packing, etc.) the whole binary was built with — the
foundation every later function match iterates from.

## In-scope translation units (likely linked into GRUNTZ.EXE)
`deflate.c inflate.c infblock.c infcodes.c inffast.c inftrees.c infutil.c trees.c
adler32.c zutil.c` (the deflate/inflate core + the default Adler-32 checksum + shared
utils). **Probably NOT linked** (confirm during matching): `crc32.c` (only used by the
gzip path), `gzio.c`/`compress.c`/`uncompr.c` (file/one-shot wrappers the engine
doesn't use — it reads from REZ/memory). `example.c`/`minigzip.c` are sample programs,
never linked.

## Rule: keep the upstream source pristine
For a clean match our source must EQUAL upstream zlib 1.0.4 — do not edit
`vendor/zlib-1.0.4/`. Anything the match needs (defines, packing, calling convention)
belongs in the **build configuration**, not the source. (Cross-check function bodies
against the FID/cross-game results in `docs/libraries-and-funcid.md`.)

## Calibrated flag set (adler32, iteration 1)
**`cl /c /O2 /MT <file>.c`** (cdecl). `/Ox` is byte-identical to `/O2` for adler32.

- Target `adler32` = `FUN_005882d0` (RVA 0x1882d0 / VA 0x5882d0), **296 bytes, 124
  instructions**, self-contained (zero relocations). Lives in delink seg
  `build/delink/target/seg_0018.cpp.obj` at .text offset 0x5c18.
- `/O2 /MT` (and `/Ox /MT`) produce the function **byte-exact** over all 296 bytes
  (the obj symbol spans 304 bytes only because of 8 trailing `nop` inter-function
  alignment pad, which is not part of the body). objdiff reports
  **matched_code_percent = 100.0, fuzzy_match_percent = 100.0**.
- `/O1`, `/Os`, `/Od` do **not** match: they emit a `push ebp; mov ebp,esp` stack
  frame and a different s1/s2 register allocation. All opt levels keep `% 65521` as a
  literal `mov reg,0xfff1; div reg` (MSVC5 never strength-reduces this unsigned
  modulo) — so the literal-divisor cue alone does NOT distinguish opt levels; the
  frameless layout does, and only `/O2`/`/Ox` produce it. (The "try /O1//Os first"
  calibration hint was disproven empirically.)
- `/Gy`, `/GF`, `/Zp1|/Zp4|/Zp8` have **no effect** on adler32 (no statics referenced,
  no structs), so they're not constrained by this function — leave at defaults until a
  function that exercises them is matched.

Artifacts: `build/zlib-cal/` (per-flag `.obj` + extracted `code_*.bin` + disasms +
`compare-table.txt`); objdiff project `build/objdiff/adler32-only/` (target
`build/delink/target/adler32_only.obj` = the extracted 296 target bytes as `_adler32`,
base `build/delink/base/seg_0018.cpp.obj` = `cl /O2 /MT adler32.c`).

## Status
Source vendored. **Step 5 (flag calibration) started: adler32 byte-matches at `/O2 /MT`
(100% objdiff).** Next: confirm the same flags hold on a function that uses statics /
structs (e.g. a deflate/inflate TU) before locking `/O2 /MT` as the global build flags.
