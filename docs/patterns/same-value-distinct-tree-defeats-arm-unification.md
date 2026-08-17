# Reading the SAME value through a DIFFERENT lexical tree keeps both arm stores alive

tags: cpp:branch cpp:if cpp:local cpp:struct | asm:jne asm:jmp asm:mov | topic:codegen-idiom
symptoms: retail's two-arm `if/else` assigning one variable emits BOTH stores plus a join
`jmp` (branch count one HIGHER than ours) while our identical-looking if/else collapses to
the skip-form (`jne over; sub; store once`); `walls diagnose` shows base one branch short
with the same calls/rets; the arm values are provably equal at runtime
confidence: 8/10
variants: identical-arms-need-distinct-locals.md, dead-second-field-load-is-a-struct-copy.md

`clippedW` in `FontRenderer::DrawGlyphRun` 0x179e70:

```cpp
// NO - both arms read gm.width (one C1 tree): cl value-unifies the diamond into
//      jne-over-subtract, ONE store, 39 branches (retail 40)
if (ci == endChar - 1) clippedW = gm.width - rightPartial;
else                   clippedW = gm.width;

// YES - arm2 reads g.width (the GetGlyph-filled original gm was copied from;
//       identical value, DIFFERENT tree): both stores + join jmp survive,
//       40/40 branches, 10/10 calls - the retail skeleton
if (ci == endChar - 1) clippedW = gm.width - rightPartial;
else                   clippedW = g.width;
```

The unit of C1's value-unification is the lexical tree, exactly as in
[identical-arms-need-distinct-locals](identical-arms-need-distinct-locals.md) - but at
EXPRESSION level, not statement level. When a struct local was copied from a by-ref
out-param (`Glyph gm = m_font->GetGlyph(g, c);`), the dev had TWO equivalent spellings
for every field and mixed them; transcribing all uses through one name is what merges
the diamond. Which arm had which spelling is not recoverable from the bytes (both
collapse to one register); pick either, keep the diamond.

Same mechanism, second site: `CTileActionEvent::SetActionCode` 0x112da0 - walk1 via
`layer` built from the raw global (`g_gameReg->m_world->m_level->m_mainPlane`), walk2
via `reg->m_world->...` where `reg = g_gameReg`. The two chains are distinct trees, so
neither CSEs into the other (both walks emitted, as retail), while the global LOAD
itself is one tree and CSEs (one `mov reg,ds:g_gameReg`, retail's count). Spelling both
walks through the same base collapses them (95.06 -> 83.78 measured by a previous
lane); the double-global device scores 95.06 but emits a second global load retail
does not have. Truth: 94.08, one global load, both walks.
