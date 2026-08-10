# A reloc-ADDEND row is usually a NAME split, not a wrong element - resolve to the address

**Tags:** data:objdiff cpp:global cpp:array cpp:loop | asm:cmp asm:lea | topic:tooling topic:scoring-artifact

## Symptom

`python -m gruntz.audit.reloc_addends` reports "same symbol, same reference count,
different offsets" - e.g. base `?g_battlezLastMaxGruntz@@3PAHA + 0x10` where retail
has `+ 0x0` - and the function looks like it indexes the wrong element of a
file-scope array. Reading the two sides' bytes shows the instruction is
**identical after relocation**.

Measured 2026-08-10 on the tree-wide four-row census: **two of the four rows were
this**, i.e. the naive per-symbol comparison ran at a 50% false-positive rate.

## Mechanism

A relocated reference is a pair `(symbol, addend)`, and the two sides do not agree
on where to put the split. `cl` names the symbol the **source** named and puts the
rest in the addend. The delinker names whatever symbol **starts at** the absolute
address and uses addend 0 - and for an address it cannot attribute at all it
invents `DAT_<rva>`.

The commonest producer is a **one-past-the-end loop sentinel**. `for (i = 0; i < 4; i++)`
over `i32 arr[4]` strength-reduces to a pointer compared against `&arr[4]`:

```
; ours              cmp edi, <DIR32 ?g_battlezLastMaxGruntz@@3PAHA> + 0x10
; retail  81 ff 10 9d 62 00   cmp edi, 0x629d10
```

`0x629d00 + 0x10 == 0x629d10`, and `g_savedDlgWndProc` is the datum that *starts*
there - so the delinker attributes retail's reloc to `g_savedDlgWndProc + 0`. Same
four bytes, two different names. The neighbouring array does it too:
`g_battlezLastDifficulties + 0x10 == g_battlezLastMaxGruntz + 0`, which is where the
census's *other* half came from. Interior biases do it as well: a loop cursor cl
folds to `&rects[0].bottom` is `g_levelMsgRectsB + 0xc`, which the delinker calls
`DAT_0020b904`.

## The fix (in the tooling, never in the source)

**Resolve both sides to an absolute retail RVA and compare THOSE.**
`build/gen/symbol_names.csv` maps name -> rva for every named symbol, and the
delinker's `DAT_<hex>` / `$gap_<hex>` placeholders carry their rva in the name.
`gruntz.audit.reloc_addends` now does this per function: it builds the multiset of
resolved addresses on each side, subtracts, and only an addend landing in the
*difference* counts as a defect. Rows where every differing addend resolves to an
address both sides reach are re-classed **`NAMING`** and are not work.

Two properties keep it honest, and both matter:

- A name that resolves on neither side (string constants `??_C@`, `__imp__*`
  thunks, the normalizer's `$Sdata_bss_*` renames) keeps the by-name comparison.
- An address that resolves on **only one** side stays classified as a defect. The
  pass never invents a pairing, so it can only remove a false positive, never hide
  a real one.

Adjudication is **per addend**, not per row: `LevelMsgHudDriver`'s base `+0x80`
sentinel pairs away while retail's `+0xc` cursor bias does not, and the row prints
both.

## Do not confuse with the real thing

[reloc-addend-is-masked-diff-the-addends](reloc-addend-is-masked-diff-the-addends.md)
is the genuine defect this class hides behind: `BuildColorChannelTables` really did
address every LUT entry one slot high. The discriminator is exactly the address
test - a real wrong-element row reaches an address retail never reaches.

`global-reference-count-sieve` learned a weaker form of the same lesson (drop an
addend past the end of the symbol it names). Bound by extent it is a heuristic;
resolving to the address is the decision procedure.
