# objdiff masks the reloc ADDEND, not just the symbol - diff `g_tbl + K` too

**Tags:** data:objdiff cpp:array cpp:global | asm:mov asm:lea | topic:tooling topic:mis-model

> **Current scoring:** the project now uses `functionRelocDiffs=all`, and its
> pinned objdiff additionally compares absolute `DIR32` addends. The historical
> blind spot below remains useful as the reason for the independent audit and for
> reading raw COFF, but these mismatches now cost the function score.

## Symptom

A function that indexes a file-scope array scores clean, or near-clean, while it
addresses the **wrong element** at run time. Nothing in `--diff`, `gruntz walls diagnose --asm`
or `gruntz walls diagnose` shows it, and the reloc-SYMBOL sequence
([reloc-sequence-diff-finds-wrong-referents](reloc-sequence-diff-finds-wrong-referents.md))
agrees perfectly: both sides reference `g_tbl` in the same order and the same number
of times.

The bug lives in the number that is *added* to `g_tbl`.

## Mechanism

objdiff treats a relocated operand as matching when the relocation's **symbol**
matches, because the linked-address bytes are meaningless before the link. But in
COFF an `IMAGE_REL_I386_DIR32` stores its **addend inline in the displacement
field**, and that field is inside the masked operand. So

```
mov word ptr [esi + 0x20000], dx     ; reloc -> ?g_clut@@3PAEA   (base)
mov word ptr [esi + 0x1fffe], dx     ; reloc -> ?g_clut@@3PAEA   (retail)
```

score identical. `g_tbl + 0x20000` and `g_tbl + 0x1FFFE` are the same symbol, and
the 2 that separates them is a whole array element. This is the *only* operand class
where a wrong constant costs nothing - a non-relocated immediate changes the encoded
bytes and does show up in the %.

Cross-check: `--target` prints the delinked ABSOLUTE address, so subtracting the
symbol's retail VA recovers retail's true addend by hand.

## How to sweep it

Per function, extract the ordered list of `(reloc type, symbol, addend)` from
`llvm-objdump -dr` on `build/objdiff/base/<u>.obj` and
`build/objdiff/target/<u>.c.obj` - the addend is the 4 bytes at the relocation's
offset inside the instruction - and compare. Three row kinds come out:

- **same symbol, different addend** - the real find. Investigate every one.
- **same multiset, permuted order** - an operand-evaluation-order difference
  (`a | b | c` reassociation), not a bug.
- **different symbol** - a wrong referent, or just the delinker naming an interior
  address after a neighbouring global.

Tree-wide this is a *small* report (35 rows over 339 units on the 2026-08-10 tree),
so it is cheap to run and cheap to triage.

## Worked example (a live rendering defect)

`BuildColorChannelTables` 0x13f740 fills `g_clut`, the 3 x 32768-entry alpha-blend
LUT. The reconstruction incremented the byte cursor **before** its three stores:

```cpp
do {
    base += 2;                              // WRONG - retail increments after
    i32 sum = varD / 32 + bDiv;
    ClutStore16(0x20000 + base, ...);       // -> reloc addend 0x20000
    ClutStore16(base,           ...);       // -> reloc addend 0x00000
    ClutStore16(0x10000 + base, ...);       // -> reloc addend 0x10000
    varD += stepA;
} while (--k != 0);
```

Retail's three stores carry addends `-2 / 0xFFFE / 0x1FFFE` against an `esi` cl has
already advanced (cl hoists the induction increment above the stores and compensates
the displacements), i.e. the source increments **after** the stores. Every entry of
all three regions therefore sat one slot high, and the last red store ran two bytes
**past the end of `g_clut`**. Moving `base += 2` below the stores reproduced retail's
addends exactly and took the function 96.86 -> **100.00 EXACT**.

## Trap: the compensating reader

Look for a *consumer* that was tuned against the broken table. `CDDSurface::ShadeRect`
0x13f460 read the same LUT at `0x10002 / 0x2 / 0x20002` - a hard-coded `+2` that
cancelled the writer's `+2`, so that one path rendered correctly and nothing pointed
at either function. Retail has neither `+2`. Fixing one without the other makes the
visible output *worse*, so sweep the whole consumer set of a table before editing it
(see [compensating-error-signatures](compensating-error-signatures.md)).
