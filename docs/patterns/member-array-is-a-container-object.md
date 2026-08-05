# A member array whose fill+access won't schedule right is a CONTAINER OBJECT, not a raw array

tags: cpp:array cpp:member cpp:class cpp:inline cpp:template | asm:pop asm:mov asm:stos | topic:codegen-idiom topic:regalloc
symptoms: a function over a member array is byte-complete — same size, same instruction
multiset, every immediate right — but ONE register op (typically an epilogue `pop`) sits at a
different point in a store run, and no statement-level rewrite moves it
confidence: 9/10

A raw member array (`u32 m_keyTable[0x20]`) written through a local cursor
(`u32* p = m_keyTable;`) and filled by a hand-written loop gives cl a *pointer* to
schedule around. The same storage declared as a **small class holding the array**, with
the fill and the element access as **in-class (implicitly inline) methods**, gives cl a
*sub-object* instead — and it emits the enclosing function's register releases in a
different order. Layout, addressing and every emitted instruction are otherwise identical,
so nothing in a `--diff` points at the declaration.

```cpp
// NO - one instruction off, and unreachable from any statement spelling:
class CInputDevice {
    u32 m_keyTable[0x20];
};
void CInputDevice::SetupKeyTable() {
    u32* keyTable = m_keyTable;
    for (i32 i = 0; i < 0x20; i++) { keyTable[i] = 0; }
    keyTable[0] = 0x20;  m_keyTable[1] = 0x11;  /* ... */
}

// YES - byte-exact:
class CKeyTable {
public:
    void Clear() { for (i32 i = 0; i < 0x20; i++) { m_keys[i] = 0; } }
    u32& operator[](i32 i) { return m_keys[i]; }
    const u32& operator[](i32 i) const { return m_keys[i]; }
    u32 m_keys[0x20];
};
class CInputDevice {
    CKeyTable m_keyTable;
};
void CInputDevice::SetupKeyTable() {
    m_keyTable.Clear();
    m_keyTable[0] = 0x20;  m_keyTable[1] = 0x11;  /* ... */
}
```

**`CInputDevice::SetupKeyTable` @0x133c30: 94.59% -> 100 EXACT, taking DinMgr2 to 60/60.**
A `template<i32 N> class TKeyTable` with the same members is **byte-identical** to the plain
class, so the bytes cannot arbitrate template-vs-class — pick whichever the module's sibling
containers use (here `CFixedPtrArray32`, a plain class, so plain class).

## Why you cannot find this by rewriting statements

The residue was one `pop edi`: retail's else-arm pops it BETWEEN the 3rd and 4th store, ours
ahead of all four — and retail is *asymmetric*, its if-arm popping after all four (which we
already matched). Everything below was measured **byte-identical** to the failing form, i.e.
a total dead end: the permuter (40 iters), 30 AST + TU-state variants, all five `return`
placements, `memset` vs the hand loop, a `goto` join, an arrow base pointer, and inline
member helpers in both arms / in one arm only / wrapping just the last store. Strictly worse:
free-function helpers taking a pointer (75.7), a table-driven inline member (60.0).

Flags are not it either, and this kills a tempting misattribution: `/G3`, `/G4`, `/G5`, `/GB`
and the default all emit the unit **identically** (only `/G6` differs, and it regresses). A
386 target has no dual pipeline to schedule for, so if a processor-targeted *pairing
scheduler* produced the interleave those could not agree — **do not attribute an
instruction-transposition residue to Pentium pairing.** `/Ob1 /Ot /Gy- /Gf` are likewise
codegen-identical; `/Ob0 /Oy- /Oi- /Os /Og- /O1` all regress.

## When to reach for this

Trigger: a member array (fixed size, filled then indexed) in a function that is byte-complete
except for register-op PLACEMENT, after statement-level rewrites have come back inert. The
fix is a *declaration* change, so it is invisible to every search that mutates the function
body — the permuter and `match_variants` cannot reach it by construction.

Related: [`ehvec-member-array-not-adjacent-fields.md`](ehvec-member-array-not-adjacent-fields.md)
(the other direction — adjacent fields that are really an array member),
[`single-return-variable-pins-accessor-regalloc.md`](single-return-variable-pins-accessor-regalloc.md)
(another "already byte-faithful, only allocation differs" case with a source lever).
