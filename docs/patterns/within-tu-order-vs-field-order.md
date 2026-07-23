# Simple TU reorder/removal is often byte-neutral — struct FIELD order is load-bearing
tags: topic:tu-layout topic:scoring-artifact topic:regalloc cpp:struct cpp:member
symptoms: whole unit stuck 70-99% 0 exact; a small-function reorder changed nothing; or a
  member-access displacement byte differs across a family
confidence: 8/10
variants: tu-completeness-rva-order.md, preceding-function-state-recolors-later-comdat.md

objdiff pairs base↔target functions **by symbol name** and scores each independently;
MSVC5 /O2 emits every function as its **own COMDAT `.text`** (base `channelslots.obj`
= 4 separate `.text`; EH units add per-fn `.text$x`/`.xdata$x`). This makes many simple
function reorder/removal experiments byte-neutral, but independent sections and
per-symbol scoring do **not** prove independent compiler state. A substantial preceding
definition can re-color an unchanged later COMDAT; see
[preceding-function-state-recolors-later-comdat.md](preceding-function-state-recolors-later-comdat.md).
The unconditionally order-sensitive case remains **struct field order**: it moves member
OFFSETS, so every `mov reg,[this+disp]` displacement byte changes.

Controlled experiments (rebuild + `llvm-objdump -dr` before/after, all reverted):
- **Fn reorder, base** (`channelslots`, swap 2 of 4): bytes byte-identical, 4/4 100% held.
- **Fn reorder, /GX EH** (`logicworkerhandlers`, move handler to front): per-fn bytes
  identical (EH state is per-function), 4/4 100% held — refutes "EH-state numbering
  carries across the TU".
- **Fn reorder, zlib C** (`trees`, swap `bi_reverse`/`bi_flush`): 19/19 100% held.
- **Data-def reorder** (`trees`, swap `extra_lbits`/`extra_dbits`): no change (`.data`
  isn't even weighted into the headline here).
- **Completeness** (`channelslots`, delete 1 of 4): the 3 survivors stayed byte-identical
  for that small-function probe. This is not a universal set-independence result.
- **Counterexample** (`ddsurface`, add the substantial earlier `BlitIntoDesc`): unchanged
  `ShadeRect` changed two inner-loop schedules and 68.4455%→63.6682%. Removing only that
  definition under current headers restored the former COMDAT shape.
- **Struct FIELD reorder** (`deflate_state`, swap adjacent `ulg opt_len`/`static_len`):
  `trees` dropped **19/19→14/19, code 100%→72.7%**; the 5 regressed fns are exactly those
  reading those fields (`init_block`/`_tr_flush_block`/`build_tree`/`gen_bitlen`/
  `build_bl_tree`), each ~99.9% (one displacement byte).

```cpp
// OFTEN NEUTRAL — verify; independent COMDATs do not imply independent compiler state:
void A(){...}  void B(){...}   // fn order
int tbl1[]={...}; int tbl2[]={...};   // static-data def order
// LOAD-BEARING — field order sets offsets; keep it exactly as retail:
struct S { u32 opt_len; u32 static_len; };  // swapping these shifts every [this+disp]
```
```asm
mov  dword ptr [esi+0x5c8], 0   ; opt_len ; swap the two fields and this
mov  dword ptr [esi+0x5cc], 0   ; static_len ; becomes 0x5cc / 0x5c8 -> byte diff
```
Default WALL/no-op: a blind reorder is not a justified lever. First fix the function and
field layout; then use a controlled A/B or state-trial test if the source-identical symbol
moved after TU composition changed. Source order also sets the function's **RVA in a full
LINK** (`docs/link-order-investigation.md`), so keep evidence-backed retail order. Never
reorder proven owners or add fake siblings only to select optimizer state.
