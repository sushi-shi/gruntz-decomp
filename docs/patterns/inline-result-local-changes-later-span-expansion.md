# A byte-flat inline result local can change later expansion decisions

tags: cpp:inline cpp:local cpp:loop | asm:call asm:mov | topic:codegen-idiom topic:wall

## Controlled observation

On PR #79 at `237f430ec` with the retained `HasReceivedThrough` helper,
`CMinimap::BuildHighOnSweetzPalette` (`0xa5d90`) emits 613 instructions and
24 calls, versus retail's 615 instructions and 24 calls. Its first difference
is at `+0x42f`, during the final packed-colour calculation. The preceding RGB
calculations already agree. This is not the older, much lower-scoring RGB
reconstruction described in the initial packing notes.

These two forms of the existing, used `Pack(i32, i32, i32)` helper produce
the same Sweetz machine body with the retained explicit fill loops:

```cpp
return static_cast<u16>(
    ((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
```

```cpp
u16 color = static_cast<u16>(
    ((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
return color;
```

A second, disposable source hypothesis exposes a difference. Mark the existing
same-TU `CMinimap::FillSpan` definition `inline`, leaving it in its existing
position before Sweetz. Replace Sweetz's 16 explicit range loops by calls to
that inclusive-endpoint helper, preserving every range and colour. Keep the
24 existing calls and all singleton `SetTileColor` calls; remove the unused
loop index. Thus every variant below has the same 40 source-level span sites.

| Pack body | FillSpan loop body | Sweetz instructions | Remaining span calls |
| --- | --- | ---: | ---: |
| Direct return | Direct table store | 633 | 13 |
| Direct return | Existing `SetTileColor` call | 632 | 15 |
| Named `u16` result | Existing `SetTileColor` call | 612 | 26 |
| Named `u16` result | Direct table store | 612 | 26 |

Every row emits the standalone `FillSpan` COMDAT. The direct-store and nested
setter bodies both retain its exact 50 bytes / 20 instructions. The gap tool
positively identifies a locally defined inline candidate; this is not an
inference from an undefined symbol. The differing calls belong to the straight
line span sequence, not duplicated control-flow tails.

Two further controls do not close the caller: an `i32` packed result followed
by the `u16` return cast gives 26 calls but changes the RGB prefix; reusing the
three channel parameters for their shifted values gives 28 calls and enlarges
the frame from 76 to 80 bytes. Moving the caller's buffer initialization to
its first use, separating all colour declarations from their assignments, and
using an expression macro for `Pack` leave the original first divergence.

## Interpretation and safe use

Machine-code equivalence of one inline spelling is not proof that it is
interchangeable inside a larger inline candidate family. Test the composition
with real consumers before dismissing a result local or nested setter as
irrelevant. The observed expansion decisions are evidence; this experiment
does **not** measure `cb` or distinguish a cost change from another front-end
decision mechanism. Use the inline-model titration before assigning numbers.

None of these variants is exact. In particular, 26 calls is not the required
24, and none repairs the original first divergence. Uniform `FillSpan` use
therefore remains a hypothesis, not recovered source provenance. The uniform
member-inlining experiments were removed. The user-approved range abstraction
is retained separately: every previously expanded range uses the typed free
`FillTileColors` helper beside `SetTileColor`, while the existing standalone
member calls remain. This uniformly represents buffer fills without claiming
that VC5 originally selected these sites from one member-inline population.
Do not retain probes, hand-select per-site helper spellings, or claim a
recovered match from this control.

See also [the inline-budget controls](inline-budget-emits-ool-comdat.md) and
[the exact singleton-setter closure](inline-singleton-setter-restores-store-call-interleaving.md).
