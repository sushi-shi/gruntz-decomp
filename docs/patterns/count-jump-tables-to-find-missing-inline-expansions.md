# COUNT THE JUMP TABLES: retail-vs-base table count tells you how many inline expansions you are missing
tags: cpp:switch cpp:inline cpp:method | asm:jmp | topic:codegen-idiom topic:wall-refuted
symptoms: a big blitter/dispatcher stuck at 20-35%; its note blames "an MSVC5 /Ob1 inline heuristic that cannot be steered from source"; retail's reloc list has MORE distinct `switchdataD_` addresses than the base COMDAT has `jmp dword ptr [reg*4+T]` sites
confidence: 9/10

A helper that contains a `switch` and is *inlined* at some call sites leaves one
jump table per inlined site, all inside the caller's own COMDAT. So the table
count is a **direct, cheap census of the inline expansions** and it needs no
disassembly reading:

```sh
# retail: how many distinct switch tables does the function own?
gruntz sema disasm <rva> | grep switchdata | sed 's/.*-> //' | sort -u | wc -l
# base: how many indexed jumps does our compiled body emit?
llvm-objdump -d --section=.text build/objdiff/base/<unit>.obj \
  | awk 'index($0,"?Fn@Class@@")&&/>:$/{f=1} f&&/Disassembly of section/{exit} f' \
  | grep -c 'jmp.*\*(,%e'
```

If retail has N and we have M < N, we are **calling** the helper at N-M sites
where retail inlined it. The table SIZES disambiguate which helper: a
`switch (m_drawType)` with cases 2,3,7,8 gives a 7-entry (0x1c) table, one with
cases 2..11 gives a 10-entry (0x28) one, and their ORDER in `.text` follows the
order of the call sites in the source.

FIX: **write the code inline at that call site.** There is nothing to steer -
`__inline` is not required and no flag is involved; the original source had the
body spelled out (or behind a macro) at those sites. Copy the existing inline
expansion and rename its locals to the site's.

`CDDrawShadeBlit::BlitLoop` 0x14a200 - retail 4 tables (0x54b6f4/0x54b710
full-width pair, 0x54b738/0x54b754 the two LEFT-clip vertical-double runs), base
had 2. Expanding both left-clip `ConvertRowDoubleFwd` calls: **24.6 -> 33.7%**, and
the base code length came to within 36 B of retail's 0x14f3.
`CDDrawShadeBlit::BlitMode_14b770` 0x14b770 - retail 3 (0x54c990 the left-clip run
FIRST, then the full-width pair), base had 2: **24.7 -> 28.7%**. Both had carried
an `@early-stop` note asserting the split was an uncontrollable /Ob1 heuristic.

Do this census BEFORE reading a single instruction of a big switch-heavy function:
it is two shell commands and it converts "regalloc wall" into a concrete edit.
Pair it with [[rva-extent-must-include-switch-tables]] - a function with tables is
also the family most likely to have a truncated `RVA()` span.
