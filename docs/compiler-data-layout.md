# MSVC 5.0 data layout: how cl orders globals in .data / .rdata / .bss

Status: INVESTIGATION IN PROGRESS (lane/msvc-data-layout). Facts below are
probe-proven against our exact toolchain (`CL.EXE` 11.00.7022 driver, `c1xx.dll`
front end, `c2.exe` 11.00.7303 back end) with the real unit flags
`/nologo /c /O2 /MT /GX /GR`. Probe corpus + capture scripts: session scratchpad
(`probes/p*.cpp`), method reproducible from this doc.

## The architecture of the decision (proven)

The front end (`c1xx.dll`) and back end (`c2.exe`) communicate through four temp
files (`*ex` expressions, `*gl` global IL, `*in`, `*sy` symbol refs). Capturing
them mid-compile (poll-copy the wine TEMP dir) shows:

1. **Initialized data (`.data`, `.rdata`) records are written into the IL stream
   at the point of the definition** — c2 emits section bytes as records arrive.
   => **declaration order**, byte-for-byte. (Probes: 16 initialized ints come out
   0x00..0x3c in declaration order; same for `extern const` in .rdata; same with
   wildly mixed sizes/types.)

2. **Uninitialized data is deferred to an end-of-TU walk inside c1xx** — the
   captured `gl` stream already lists the records in the final `.bss` layout
   order; c2 just allocates offsets in arrival order. The walk order is a
   function of the SYMBOL NAMES (hash-table walk), NOT of declaration order.

3. **One unified walk** covers: extern uninitialized globals, file-`static`
   uninitialized globals, and function-local `static` uninitialized objects
   (under their plain source names — `s_cnt`, not the decorated
   `_?s_cnt@?1??counter@...$S169`). All interleave in a single ordering.
   (Probe p12/p13.)

4. **C vs C++**: compiled as C, uninitialized globals become COFF COMMON
   symbols (sec 0) — no section placement at all in the object; the linker
   allocates them. The COFF symbol-table order still shows the same name-walk
   (reversed). Gruntz is C++, so per-TU `.bss` blocks come from mechanism 2.

## The name-walk (partially characterized, binary read pending)

Within families of same-length names differing in trailing characters, the
bucket index behaves like `h = h*4 + c` over the characters (weight 4 per
position): probe `int g_a00..g_a15` emits
`00 01 02 03 10 04 11 05 12 06 13 07 14 08 15 09` — i.e. ascending `4*X+Y`
with LIFO collision chains (`g_a10` collides with `g_a04` and, being declared
later, is emitted first). A 400-name capture (`q_000..q_399`) confirms weight
16/4/1 for three varying digit positions, with LIFO ties throughout.

- The hash input is the **undecorated identifier**: mixing `extern "C"` and C++
  linkage, or varying the type (=> wildly different decorated names), does not
  change the relative order.
- A 28-name mixed-length probe REFUTES every simple
  `h = h*m (+|^) c (mod M)` / rotate-xor family under an
  ascending-bucket + LIFO-chain emission model (exhaustive fit over
  m ∈ {2..65599}, M ∈ 2..65536, 16/32-bit wraps, both walk directions, both tie
  policies). The real mechanism has more structure — being read out of
  `c1xx.dll` (`symtable.cpp` / `toil.c`) now.

## Alignment (probe evidence so far, rule still open)

- `.bss` and `.data` place every object at >= 4-byte alignment (two adjacent
  `char` globals sit 4 apart in BOTH sections).
- `double` => 8.
- char arrays: size 5/8/16 got 8-aligned in one .bss probe; but `char[5]`,
  `char[7]` got only 4-aligned in a .data adjacency probe, while `char[9]`,
  `char[17]` got 8-aligned there; `char[12]` in .bss got 4. Neither
  "8 iff size >= 8" (previous lane) nor "8 iff size > 4" survives all probes;
  the real rule is type-shape-dependent and/or stateful and will be read from
  the binary, not fitted.
- The delinker's synthetic `8 iff size % 8 == 0` rule matches NONE of this and
  is definitely wrong.

## Back-end facts (from reading c2.exe, UTC "P2" lineage)

- `c2.exe` carries assert paths `E:\utc\src\P2\*.c` — it is the UTC back end
  (`p2symtab.c`, `coff.c`, `coffemit.c`, `emit.c`, `reader.c`, `hash.c` = CSE
  value-number hashing, not names).
- Symbol records are read by ordinal from the IL (`reader.c` /
  `p2symtab.c` FUN_004206b7); c2 does not re-order data symbols.
- Segment-class -> COFF section mapping at 0x41edbc..0x41ee52: class 2 =>
  `.data` (0xC0000040), 3 => `.rdata` (0x40000040), 4 => `.bss` (0xC0000080).
