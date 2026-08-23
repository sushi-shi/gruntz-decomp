# Retail recomputes a loop-invariant index: read it through the ESCAPED object, not the copy

tags: cpp:loop cpp:local cpp:call | asm:imul asm:mov | topic:codegen-idiom
symptoms: retail's INNER loop recomputes a whole address every iteration
(`mov edx,<outer counter>` / `imul edx,[stride]` / `add edx,<base>`) while ours reads it
through one register the OUTER loop advances by the stride; `walls loopscan` reports the
inner body as retail-FATTER with `retail holds: imul` and the outer body as ours-fatter by
the same strength-reduction bookkeeping
confidence: 8/10

## Shape

A nested blit/scan whose inner loop indexes a 2-D buffer:

```cpp
buf[row * stride + col]
```

Retail emits the multiply and the add INSIDE the inner loop, once per pixel. cl 5.0 in our
build instead proves `row * stride` invariant across the inner loop, hoists it, and
strength-reduces it into an outer-loop induction variable (`add ebp,<stride>` at the bottom
of the outer body). The inner loop then costs one `cmp BYTE PTR [reg+idx],0` where retail
costs five instructions.

The instinct is to call this a scheduling coin, or to "fix" it by hand-hoisting a row
pointer. Both are wrong: cl 5.0 hoists whenever it is ALLOWED to, so retail not hoisting is
evidence that in retail's source the hoist was **illegal**.

## Cause

The inner loop STORES through a pointer. cl may only hoist `row * stride` if it can prove
that store cannot write `stride`. It can prove that for a local whose address never escaped;
it cannot for a local that has been passed by reference to anything.

`FontRenderer::DrawGlyphRun` 0x179e70 had BOTH forms of the same value in scope:

```cpp
Glyph  g;                                     // address escapes into GetGlyph
Glyph  gm = m_font->GetGlyph(g, text[ci]);    // a copy; address never escapes
...
u8 cover = glyphBuf[row * gm.width + col];    // <- hoisted: gm never escaped
```

`Font::GetGlyph(Glyph& out, u8 c)` returns `Glyph&` bound to `out`, so `g.width` and
`gm.width` are the same number and the two spellings are behaviourally identical. Only the
provenance differs. Indexing through `gm` let cl hoist; indexing through `g` - whose address
went into `GetGlyph` and which the inner loop's `u16*` store may therefore alias - forces
retail's per-iteration recomputation in all three inner loops at once.

One character of source: **70.89 -> 78.04**, and the two remaining inner-loop deltas
(`7 vs 11` and `29 vs 32`, both `retail holds imul`) closed to `10 vs 11` and `30 vs 32`.

## Detection

`gruntz walls loopscan <rva>` and read the direction. A row where retail's body is FATTER
and the surplus mnemonic is `imul` (or `lea`+`add` against a scaled index) is this pattern.
It is the reverse of the usual "ours is fatter" reading, so a one-directional sieve misses
it entirely - run loopscan both ways.

Corroborate before editing: the outer loop should show the mirror image, ours carrying an
`add <reg>,<stride>` that retail does not have. That pair - retail fatter inside, ours
fatter outside - is the strength-reduction signature and distinguishes it from a genuine
extra statement.

## Do not generalise it into "cache the member"

The inverse move is not a lever. `CRezImage::FlipVertical` 0x176840 shows the same
retail-recomputes-inside shape, and caching `m_height` into a local to try to reproduce
retail's frame-slot read took it **71.07 -> 56.78**. Its residue is a third outer induction
variable retail carries and we do not, which is a different problem. Only reach for this
pattern when the value already exists in the function under two names, one of which has
escaped.

## Related

- [`retail-recomputes-a-shift-we-cse`](retail-recomputes-a-shift-we-cse.md) - the same
  "retail does not CSE what we do" symptom on a `sar`, where every spelling still CSEs.
  That one is a wall; this one is not, because escape analysis is reachable from source.
- [`2d-array-codegen-signature`](2d-array-codegen-signature.md)
