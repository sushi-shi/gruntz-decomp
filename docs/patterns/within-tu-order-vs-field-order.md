# Reordering a TU's functions does NOT change the objdiff % — struct FIELD order does
tags: topic:tu-layout topic:scoring-artifact topic:regalloc cpp:struct cpp:member
symptoms: whole unit stuck 70-99% 0 exact; tempted to reorder defs to ascending-RVA to "fix" it; reorder changed nothing; a member-access displacement byte differs across a family
confidence: 9/10
variants: tu-completeness-rva-order.md

objdiff pairs base↔target functions **by symbol name** and scores each independently;
MSVC5 /O2 emits every function as its **own COMDAT `.text`** (base `channelslots.obj`
= 4 separate `.text`; EH units add per-fn `.text$x`/`.xdata$x`), compiled with
per-function regalloc / scheduling / EH-state numbering. So the *order* and *set* of
functions in a `.cpp` is **matching-NEUTRAL** for the per-object metric: reordering only
permutes the COMDAT sections, it does not touch any function's bytes. The genuinely
order-sensitive thing is **struct field order** — it moves member OFFSETS, so every
`mov reg,[this+disp]` displacement byte changes.

Controlled experiments (rebuild + `llvm-objdump -dr` before/after, all reverted):
- **Fn reorder, base** (`channelslots`, swap 2 of 4): bytes byte-identical, 4/4 100% held.
- **Fn reorder, /GX EH** (`logicworkerhandlers`, move handler to front): per-fn bytes
  identical (EH state is per-function), 4/4 100% held — refutes "EH-state numbering
  carries across the TU".
- **Fn reorder, zlib C** (`trees`, swap `bi_reverse`/`bi_flush`): 19/19 100% held.
- **Data-def reorder** (`trees`, swap `extra_lbits`/`extra_dbits`): no change (`.data`
  isn't even weighted into the headline here).
- **Completeness** (`channelslots`, delete 1 of 4): the 3 SURVIVORS stayed byte-identical
  — the *set* doesn't perturb sibling codegen either.
- **Struct FIELD reorder** (`deflate_state`, swap adjacent `ulg opt_len`/`static_len`):
  `trees` dropped **19/19→14/19, code 100%→72.7%**; the 5 regressed fns are exactly those
  reading those fields (`init_block`/`_tr_flush_block`/`build_tree`/`gen_bitlen`/
  `build_bl_tree`), each ~99.9% (one displacement byte).

```cpp
// NEUTRAL — reordering these never changes the % (each is an independent COMDAT):
void A(){...}  void B(){...}   // fn order
int tbl1[]={...}; int tbl2[]={...};   // static-data def order
// LOAD-BEARING — field order sets offsets; keep it exactly as retail:
struct S { u32 opt_len; u32 static_len; };  // swapping these shifts every [this+disp]
```
```asm
mov  dword ptr [esi+0x5c8], 0   ; opt_len ; swap the two fields and this
mov  dword ptr [esi+0x5cc], 0   ; static_len ; becomes 0x5cc / 0x5c8 -> byte diff
```
WALL/no-op: RVA-reordering a stuck function CANNOT crack it — the bytes don't depend on
neighbours, so a plateau is a per-function reconstruction gap (or a field-offset error),
not a layout problem. Caveat: source order DOES set the function's **RVA in a full LINK**
(`docs/link-order-investigation.md`) — that matters for a future *whole-EXE* layout match,
not for today's per-object objdiff score. Ascending-RVA ordering is worth keeping for
readability/link-layout, but it is not a %-lever.
