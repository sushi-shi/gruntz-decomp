# The `.CRT$XC` table is a COMPLETE census of the image's file-scope constructed objects
tags: cpp:static cpp:ctor | data:layout | asm:jmp asm:mov | topic:identity topic:tooling
symptoms: a 0x50-byte cell holding `e9 1b 00 00 00` + eleven 0x90 + sixteen 0xcc + a body that writes three globals and rets; a `.text` address with ZERO call sites that appears exactly once as an absolute pointer in `.data`; FLIRT `__inc` LOW rows on 5-byte jumps
confidence: 10/10
variants: static-initializer-run-reveals-record-layout.md

`_cinit` walks four `_initterm(begin, end)` pointer tables that the linker merges
into the head of `.data`. Read their bounds off the `push`/`push`/`call` pairs -
do not guess the extent from a reloc run, which spans several tables:

```asm
00121e00  a1 00 61 61 00      mov  eax,[__FPinit]      ; _cinit
          85 c0  74 02  ff d0
          68 b8 9c 60 00      push __xi_z  ; 0x209cb8  ; C initializers
          68 a4 99 60 00      push __xi_a  ; 0x2099a4
          e8 ..               call _initterm
          68 a0 98 60 00      push __xc_z  ; 0x2098a0  ; C++ initializers
          68 00 80 60 00      push __xc_a  ; 0x208000
          e8 ..               call _initterm
```

In GRUNTZ.EXE: **XC `0x208000..0x2098a0`** (1576 slots, 1075 non-null), XI
`0x2099a4..0x209cb8` (3), XP `0x209dbc..0x209fc4` (1), XT `0x20a0c8..0x20a2d0` (1).
`_initterm` skips nulls, so the interior zeros are not table boundaries - they are the
2-slot gaps between per-object contributions.

**Every dynamic initializer in the image is in XC and nothing else is.** That makes the
table the authoritative worklist for "which file-scope objects have a constructor", with
no reliance on Ghidra's function table. Its entries are unreachable by any other route:
zero `E8` call sites, zero ILT-band jumps.

## The slot points at a THUNK, not at the body

973 of the 1075 slots hold a 5-byte `jmp` cell; 102 point straight at the body.

```asm
00115c80  e9 1b 00 00 00      jmp  0x115ca0        ; the XC slot holds THIS address
          90 x11                                   ; ... pad to +0x10
          cc x16                                   ; ... linker fill to +0x20
00115ca0  b8 02 00 00 00      mov  eax,2           ; the initializer body
          c7 05 90 eb 64 00 00 00 00 00
          a3 94 eb 64 00
          a3 98 eb 64 00
          c3
```

Deltas measured across the 1075 slots: `+0x20` x958, `+0x10` x5, `+5` x10, no thunk x102.

**Corollary that kills a real mis-inference.** A `$E` initializer is never CALLED, so it
never needs an ILT-band entry. "This function has no incremental-link thunk, therefore it
came from a static `.LIB` rather than a link-line object" is undefined for anything in XC
- the thunk oracle has no input there. `SetVersionRect` 0x9fe10 / `SetMenuTextRect`
0xa1190 (`CRect g_versionRect(5, 453, 635, 478);` and its twin) were both filed that way.

## The FLIRT `__inc` family: 965 false rows, one true one

The real `_inc` is LIBCMT `input.c`'s one-character reader, `0x129da0`, 33 bytes,
`AMBIG`, sitting between `__hextodec`, `__un_inc` and `__whiteout` and tail-calling
`__filbuf`:

```asm
00129da0  8b 4c 24 04  8b 41 04  48  89 41 04  78 0a      ; --stream->_cnt >= 0 ?
          8b 11  33 c0  8a 02  42  89 11  c3               ;   0xff & *stream->_ptr++
          51  e8 e3 5c 00 00  83 c4 04  c3                 ;   : _filbuf(stream)
```

The other 965 rows were a short masked signature landing on a 5-byte `jmp` - `e9 rel32`
carries almost no entropy - at starts an older Ghidra table admitted. 963 of them were XC
initializer thunks and 2 were already-attributed compiler-helper forwards. They are
pruned. LOW rows never carved anything (`gruntz.core.library_labels.is_active`), so this
changed no denominator; it removed 965 false leads.

## Worked census: one header, 106 TUs, 954 initializers

Decoding every XC body's stores partitions the family exactly:

```text
954 bodies write three dwords at one base and ret; 9 distinct value triples,
each appearing 106 times; 106 runs of exactly 9 contiguous XC slots
```

That is `include/Gruntz/GruntDirStatics.h`'s nine `static GruntDirectionCell` objects,
one private copy per includer - and `grep -rl GruntDirStatics.h src` returns exactly 106
`.cpp` files. The census is therefore an independent oracle for
`config/static_data_copies.tsv`: it found 15 cells with no manifest row (bootycheatstate,
gruntzmgr, netsessionmgr each had the four cardinals and not the other five, because the
linker split those blocks across two `.bss` regions).

It also REFUTES the sibling pattern's "8 GruntSteps helpers + 1 scattered DirectionClassify
singleton" reading of `0x047740..0x0479c0`: those nine slots are contiguous at
`0x208904..0x208924` with the usual 2-null gap on each side, so they are one TU's nine.

Tooling, not a wall. Decode XC before inventing an owner for an unreferenced `.text` body
or an unclaimed `.bss` triple.
