# A jump-table switch lays its arms out in SOURCE order — read the tables to recover it

- **confidence** c10
- **tags** `cpp:switch` `topic:codegen-idiom` `topic:method` | `asm:jmp`
- **measured** `CTriggerMgr::LoadTileArrivalFx` @0x75e90 **45.15 -> 61.67** (deficit -156 -> -58);
  the arm reorder alone was worth ~11 points.

## The recovery

For `movb <idx-table>(%eax) ; jmpl *<jump-table>(,%ecx,4)`, read BOTH tables out of
the image:

    jt  = struct.unpack("<%dI" % n, data[off(jump_table_rva):])     # unique targets
    idx = data[off(index_table_rva) : ... ]                         # value -> entry

`idx[i]` is the entry for `lo + i` (the `add eax, -lo` before the range check gives
`lo`). Sorting the *targets* by ADDRESS gives the order cl emitted the case bodies —
and cl emits them **in source order**, not in value order. So the address order of the
distinct arm bodies IS retail's `case` order.

`LoadTileArrivalFx`'s tables said reason 13, 5, 7, 15, 3, 18 — i.e. the devs wrote
`case PICKUP_SHOVEL / GAUNTLETZ / GOOBER / SPY / BRICK / TOOB`, nothing like the
value order our reconstruction had guessed. Reordering the arms to match re-flowed
every block in the function.

The same trick recovers a nested switch's order: the inner brick switch's targets
came out RED, GOLD, BLUE, BLACK — not the RED, BLUE, GOLD, BLACK we had.

## One entry per CONTIGUOUS run of case values, not per group

Two adjacent jump-table entries holding the SAME address is not a bug and does not
mean the source had two bodies. MSVC 5.0 emits one unique-target entry per
**contiguous run of case values**, so

```cpp
case 0x1e:            // run 1  -> entry 0
case 0x1f:
case 0x21:            // run 2  -> entry 1, same label
    body;
```

yields `jt = [L, L, ...]`. Writing the three labels as one group reproduces it.
Splitting them into two bodies (hoping cl would merge them) emits the body TWICE and
costs instructions - measured.

related:
[reloc-sequence-diff-names-the-missing-statement.md](reloc-sequence-diff-names-the-missing-statement.md),
[always-returning-gate-dce-kills-a-later-switch-arm.md](always-returning-gate-dce-kills-a-later-switch-arm.md)
