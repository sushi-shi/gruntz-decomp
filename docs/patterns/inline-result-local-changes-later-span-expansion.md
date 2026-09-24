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

None of those initial variants is exact. In particular, 26 calls is not the required
24, and none repairs the original first divergence. Uniform `FillSpan` use
therefore remains a hypothesis, not recovered source provenance. The uniform
member-inlining experiments were initially removed. The user-approved range abstraction
was first retained separately: every previously expanded range used the typed free
`FillTileColors` helper beside `SetTileColor`, while the existing standalone
member calls remain. This uniformly represents buffer fills without claiming
that VC5 originally selected these sites from one member-inline population.
Do not retain probes, hand-select per-site helper spellings, or claim a
recovered match from this control.

## Composition with the helper's real loop cursor

The follow-up at `96a42aa0d` closes the call-boundary question without selecting
different APIs for different sites. Keep the named `u16` packed-colour result
and let the existing by-value `FillSpan` parameter own its loop cursor:

```cpp
inline void CMinimap::FillSpan(u32 x1, u32 x2, u16 color) {
    if (x1 > x2) {
        return;
    }
    for (; x1 <= x2; x1++) {
        m_tileColors[x1] = color;
    }
}
```

The controlled Sweetz results on this source state are:

| Pack result | FillSpan cursor | Instructions | Span calls | Standalone FillSpan |
| --- | --- | ---: | ---: | --- |
| Direct return | Separate `u32 i` | 633 | 13 | Exact |
| Direct return | Parameter `x1` | 637 | 10 | Exact |
| Named `u16` | Parameter `x1` | 613 | **24** | **Exact** |

The first cursor change goes away from the desired call count. Composing the
independently supported result local then restores retail's 24-call boundary.
There are no inert declarations, extra operations, per-site overrides or
forced calls. Using the same member at all range sites works across all eight
palettes: seven have 24 calls and Honey has 23, exactly as retail. All eight
also have the retail branch, return and relocation counts. The standalone
helper remains 50 bytes / 20 instructions, including its `rep stosd/stosw`
fill implementation. All 131 formerly separate free-helper sites are now
ordinary member calls; the redundant free helper is removed.

An independent source audit compares all eight 500-entry symbolic colour
tables and their complete ordered write traces; every RGB constant, inclusive
range and singleton write agrees. This composition removes an arbitrary
source-level split, but is **not** a whole-palette exact closure: Sweetz still
has 613 instructions against retail's 615, and its first difference remains
in packed-colour scheduling. The other seven retain scheduling/register
residues as well. Do not confuse a complete call-boundary recovery with 100%.

Negative controls narrow the mechanism. A guarded `do/while` emits ten caller
calls but loses the standalone `rep` fill (37 bytes / 14 instructions and two
branches), so it is not an equivalent compiled helper. Reusing `first` in the
former free range helper does not repair Sweetz. Byte-sized RGB arguments
retain `sar` through integer promotion but produce 25 calls; a three-statement
packed-colour accumulation produces 26. Neither fixes the first divergence.

Real-compiler 25-site harnesses using the canonical `CMinimap` declaration
measure 13 expansions for the separate-index body and 15 for the parameter-
cursor body: under the model's floor-budget assumption, `cb` brackets are
72–76 and 63–66 respectively. The smaller 12-site harness expands everything
for the former body; that is saturation, **not** evidence of budget exemption.
The CLI's previous `cb <= 40` claim for that case is corrected and covered
through its complete compiler/measurement/reporting path by
`scripts/test_inline_model_measure.py`. These are callee measurements, not a
measurement of Sweetz's caller cost or proof of every nested budget value.

Definition-order control: the member body stays at its original TU position,
after Rocky Roadz and before Gruntziclez. VC5 still expands it in Rocky Roadz;
an explicit `inline` on the earlier class declaration is byte-flat and is not
needed. Moving the definition ahead of the first palette was also byte-flat
but violated the repository's retail TU-order gate, so that move is not kept.
Do not reorder real function owners merely to make source visibility look
more convenient.

The final family audit finds no exclusive semantic keys in any palette:
immediate and store multisets agree in all eight. The remaining ordered-
referent differences in Gruntziclez and Space are shift-global load scheduling,
not a different callee or datum. The near-exact raw-referent audit covers 3,919
functions with zero defects. `Refresh`, which uses none of these palette-build
helpers and retains its source fingerprint, moves from 69.2227% to 69.0882%;
its six calls, constants and ordered referents remain the same. This current
TU-state movement is adjudicated without lowering its bank or changing its
existing unresolved control-flow reconstruction.

See also [the inline-budget controls](inline-budget-emits-ool-comdat.md) and
[the exact singleton-setter closure](inline-singleton-setter-restores-store-call-interleaving.md).
