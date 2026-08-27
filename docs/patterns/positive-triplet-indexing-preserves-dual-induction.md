# Positive triplet indexing preserves two induction variables

- **confidence** c9
- **tags** `cpp:loop` `cpp:pointer` `cpp:array` | `asm:lea` `asm:add` `asm:mov` | `topic:codegen-idiom` `topic:regalloc`

## Symptom

A three-byte pixel loop names both a destination byte offset and a source pointer, but
VC5 collapses them to one induction variable. Retail retains two walkers: one register
indexes the destination base while another advances the source by three. The retail
loads can nevertheless use `src + 2` followed by `[-2]`, `[-4]`, and `[-3]`, which
makes a literal transcription of those emitted addresses tempting.

The source distinction is the pointer's logical base. This form lets C2 prove a fixed
bias and merge the walkers:

```cpp
u8* sp = src + 2;
dst[d] = sp[-2];
dst[d + 1] = sp[-1];
dst[d + 2] = *sp;
```

The natural pixel-start spelling preserves their separate identities:

```cpp
u8* sp = src;
dst[d] = sp[0];
dst[d + 1] = sp[1];
dst[d + 2] = sp[2];
sp += 3;
d += 3;
```

The emitted code can still schedule the first load before `sp += 3` and represent the
remaining loads with negative offsets. Therefore retail's negative displacements do
not prove that the developer based the pointer at the last byte.

## FaderEffects evidence

`CFaderShape::RenderWarpTile` 0x00181e50 began at 79.7699%, 0x77c bytes, 601
instructions, and the exact retail call/branch/return/relocation counts. Its two
sequential 24-bit loops were the only small loop-body mismatches: the leading body was
13 instructions against retail's 14, while the mirrored trailing body was 15 against
14. Although the source named `d` and `sp`, the base object coalesced their address
progression.

A 25-state Cartesian crossed five source families independently at both sites: the
existing loop, an offset-preserving whole-run helper, an interior-pointer helper, a
two-pointer helper, and positive indexing from the pixel start. The independent results
were:

| leading site | trailing site | result |
|---|---|---:|
| current | current | 79.7699% |
| current | positive indexing | 80.4149% |
| positive indexing | current | 87.9206% |
| positive indexing | positive indexing | 88.3306% |

The combined candidate is 0x78f bytes with all four relocations preserved. Its first
24-bit loop emits the retail two-walker sequence down to `lea src+2`, the three negative
loads, the separate `add dst,3`, and the decrementing count. `gruntz walls loopscan`
then pairs all 24 inner loops with retail at the same body sizes.

A second 36-state matrix separated pointer base, `const` qualification, and update
order. All negative-index forms stayed in the 79.77/80.27 compiler islands regardless
of update order or `const`; all positive-index forms reached the 87.92/88.33 islands.
The pointer base is the lever. The combined state still has one loop-entry trampoline
and is not full closure; that residual does not weaken the controlled inner-loop result.

## Reverse-use rule

1. Require a fixed-width copy where source and destination advance together and retail
   retains two induction registers while the reconstruction coalesces them.
2. Express the source pointer at the logical element start and use non-negative field
   offsets. Do not transcribe the optimizer's post-increment negative displacements.
3. Test mirrored sites independently. Coherent contributions at both sites distinguish
   an authored representation from a single-site register accident.
4. Treat `const` and statement-order variants as controls. If they occupy the same
   compiler island, do not retain them as steering syntax.
5. Re-run the full loop census. A higher score is insufficient if another loop body or
   the branch skeleton moves away from retail.
