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

## Locked global flag set (iteration 2): `cl /c /O2 /MT`
The remaining unconstrained flags (`/Gy`, `/GF`, `/Zp`) are now **bounded** by matching
the struct-/static-table-heavy zlib TUs (trees, inftrees, deflate, infblock, infcodes,
inffast, infutil, zutil) against GRUNTZ.EXE. **42 zlib functions are byte-exact** at
`cl /c /O2 /MT` (the same flags adler32 needs — no extra flags required). Evidence
below; the per-function table lives in `build/gen/bindings.tsv`.

Byte-exactness here means: extract the function's code bytes from the delinked target
obj and the freshly-compiled base obj, zero the 4-byte slots covered by relocations on
both sides (absolute addresses/symbol references differ by construction), and compare.
All 42 are identical. (objdiff's *exact* `matched_code` is lower for some — e.g. trees
58.2%, inftrees 65.7% — purely because the delinked target's relocations point at Ghidra
`DAT_*/FUN_*` names while the base points at the real zlib symbols; the **code bytes are
identical**, which is what the flag lock requires. `fuzzy_match_percent` is ~99-100% for
all, confirming this.)

- **`/Zp` (struct member packing) — packing below 4 excluded by deflate.c.**
  deflate.c is `deflate_state`-struct-heavy. Compiling deflate.c at `/O2 /MT` with
  `/Zp1`, `/Zp2`, `/Zp4`, `/Zp8`, `/Zp16`, and *default* (no `/Zp`):
  `/Zp4`/`/Zp8`/`/Zp16`/default all emit **identical** `.text`; `/Zp1` and `/Zp2`
  both differ.
  Verified against the target: `_fill_window` target-vs-default = **0 byte diffs**,
  target-vs-`/Zp1` = **30 byte diffs**; `_deflate_slow` target-vs-`/Zp1` = **87 byte
  diffs**. So the binary was built with packing >= 4 (member alignment unchanged); the
  VC5 default `/Zp8` satisfies this. `/Zp1` and `/Zp2` do NOT match; the exact
  bytes cannot distinguish `/Zp4`, `/Zp8`, or `/Zp16`. -> keep default.
- **`/Gy` (function-level linking / COMDAT) — effectively ON (forced by `/O2`).**
  At `/O2` every function is emitted in its own COMDAT `.text` section (deflate.obj has
  17, inftrees.obj 6, etc.), and `/Gy-` does not change the output (`/O2` forces COMDAT
  packaging). This matches GRUNTZ.EXE, where each zlib function appears at its own RVA
  with its own size and the TUs are laid out as independently-orderable COMDATs (the
  layout is link/COMDAT order, NOT source-definition order — see trees.c). -> default ok.
- **`/GF` (read-only string pooling) — OFF, proven (2026-08-04, supersedes the earlier
  "no effect" claim).** At `/O2` string literals already pool: each unique literal becomes
  its own **writable `.data` COMDAT** (selection `Any`), symbol `??_C@…` — that is `/Gf`,
  which `/O2` implies (`/O2 /Gf` output is byte-identical to `/O2` modulo the COFF
  timestamp). `/GF` = `/Gf` + move the literal COMDATs to read-only `.rdata`. The zlib
  base objs and retail both hold these literals in **writable `.data`**, so retail was NOT
  built with `/GF` -> leave off. (See `string-pooling.md` for the full pooling semantics.)

**Conclusion: the global compile flags are `cl /c /O2 /MT` (cdecl). No `/Gy`, `/GF`, or
`/Zp` override is needed — `/O2` already forces COMDAT, default packing is compatible,
and `/GF` is proven off by retail's writable literal COMDATs.** The build's
`CL_FLAGS = ["/nologo","/c","/O2","/MT"]`
(`scripts/gruntz/core/cc_wrap.py`) is correct and unchanged.

Artifacts: `build/zlib-cal/` (per-flag `.obj` + extracted `code_*.bin` + disasms +
`compare-table.txt`); objdiff project `build/objdiff/adler32-only/` (target
`build/delink/target/adler32_only.obj` = the extracted 296 target bytes as `_adler32`,
base `build/delink/base/seg_0018.cpp.obj` = `cl /O2 /MT adler32.c`).

## Status
**COMPLETE — every zlib TU is 100% (matched_code 100, all functions exact).**
Global flags locked at `cl /c /O2 /MT`; deflate excludes `/Zp1` and `/Zp2` while
`/Zp4`/`/Zp8`/`/Zp16` remain indistinguishable, `/Gy` is forced on by `/O2`
(confirmed by COMDAT layout), and `/GF` is proven off.

The last three functions (`_inflate`, `_inflate_blocks`, `_inflate_codes` — the big
`switch(state->mode)` state machines) were byte-exact all along but mis-measured:
Ghidra's carve ends at the code, so the delinked target dropped each function's INLINE
`.text` jump table while the base COMDAT contains it. Fix: the `size` column of their
`config/retail/functions_zlib.tsv (+ data_zlib.tsv)` rows now carries the FULL COMDAT span (`0x430`/`0xcb0`/`0x7b0`
— for each, retail next-symbol gap == base COMDAT size), which synth_pdb honors, so the
delinker carves the table into the function; normalize's jump-table reloc rewrite then
pairs the table dwords. Two `data` rows complete the reloc naming: `_border$S`
(infblock static; the table carries the CANONICAL decorated spelling — cl's `$S<n>`
CodeView counter is masked off both sides by `core.msvc_names`, so no committed row
ever states a number) and `_inflate_mask` (infutil) — without them the target
relocs bind to a neighboring string COMDAT with an addend. The vendored zlib C source
stays PRISTINE — no labels in it at all (NOT the `include/rva.h` `RVA()` macros that `src/`
uses): their static/K&R functions are dropped from IR when unused, so labels can't ride
attributes/IR. The `functions_zlib`/`data_zlib` provider tables ARE each zlib unit's rows
(no source parse, no positional join, no object consulted), and the Model joins them into
`build/gen/bindings.tsv`.
