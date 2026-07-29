# A commutative `memberA + memberB` of the SAME object: which one is loaded first is canonicalized
tags: cpp:member cpp:int | asm:mov asm:add asm:lea | topic:wall topic:regalloc
symptoms: two `mov r,[obj+N]` whose DISPLACEMENTS are swapped, the `add`/`lea` that consumes them byte-identical, ~99.98%
confidence: 9/10
variants: commutative-imul-operand-in-eax.md

Two members of the same object summed (`t->m_screenY + t->m_extent.top + dy`) lower to two
loads and one `add`/`lea`. The `add`/`lea` bytes are the same either way, so the ONLY diff is
which member's displacement is in the first `mov`. cl5 picks it in the back end; the source
expression cannot reach it.

```cpp
// all of these emit the SAME load order - the pick is not in the source:
t->m_screenY + t->m_extent.top + dy        t->m_extent.top + t->m_screenY + dy
t->m_screenY + (t->m_extent.top + dy)      dy + t->m_screenY + t->m_extent.top
i32 sy = t->m_screenY; sy + t->m_extent.top + dy   // and the m_extent.top-first twin
i32 py = t->m_screenY; py += t->m_extent.top; py += dy;   // compound form
```
```asm
base:   mov eax,[edx+0x138]  /  mov ebx,[edx+0x60]   ; then  add eax,ebx
retail: mov eax,[edx+0x60]   /  mov ebx,[edx+0x138]  ; then  add eax,ebx   (same bytes)
```
WALL. Evidence: `CGameLevel::ProbeHeadSoft` 0x160450 / `ProbeFootSoft` 0x160080 /
`ProbeFootBlocked` 0x160210 / `HoldMove` 0x15ff20 all sit at 99.98-99.99% on exactly this
4-byte pair. **It is context-, not source-, determined**: a standalone `struct AB : CGameLevel`
replica compiled from the IDENTICAL source picks retail's order for the foot pair and ours for
the head pair, so it is TU-cumulative back-end state (`match_variants --state-trials` territory,
not a spelling). ~35 expression AND statement forms tried across two harnesses; none moved it.
Distinct from [[commutative-imul-operand-in-eax]] (an `imul`'s reg-vs-mem operand).
