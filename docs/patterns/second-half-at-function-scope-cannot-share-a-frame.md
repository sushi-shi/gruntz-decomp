# A guarded first half that `return`s leaves the rest at FUNCTION scope - and its locals stop sharing slots

tags: cpp:branch cpp:local | asm:sub asm:lea | topic:codegen-idiom
symptoms: every `[esp+N]` in the diff is shifted by a constant (one or two slots), the
`sub esp,X` differs by exactly `sizeof` one local, block skeleton otherwise identical;
the function is `if (cond) { …locals A…; return 1; }` followed by more code with its own
locals
confidence: 9/10

cl 5.0 overlaps stack slots by BLOCK SCOPE, not by live range. Two groups of locals share
slots only when they sit in sibling blocks. So this:

```cpp
if (cond) {
    CString s; RECT r1, r2, r3;         // 3 RECTs + 3 CStrings
    …
    return 1;
}
CString title; RECT rTitle;             // <-- FUNCTION scope: its own slots
for (…) { RECT rA, rB; … }
return 1;
```

allocates a fourth RECT slot and extra CString dwords, because `title`/`rTitle` are at
function scope and cannot alias the `if`'s block. Retail packs both groups into the same
3 RECTs + 3 dwords, which means its source had them as **siblings**:

```cpp
if (cond) {
    …
    return 1;
} else {
    …
    return 1;
}
```

`CBootyState::ShowSecretBonusMessage` 0x18f00 (2026-08-08): the `else` alone took the
frame 0x58 -> 0x48 and the function 99.93 -> 99.995; the last 0.005 was an unrelated
argument swap. The whole function went **93.19 -> 100.00 EXACT**.

A frame-size mismatch is never noise - `python -m gruntz.audit.insn_count` and the
uniform `[esp+N]` shift in `gruntz sema disasm --diff --lite` both point straight at it.
Count the delta in units of `sizeof(the local you suspect)` before reaching for regalloc
explanations.

The same rule applies to a construct expanded N times in one function, which is how it shows
up in the Battlez pathing code: `CMapMgr::Clip` is inlined three times in
`CBattlezMapConfig::RepathAroundBlockedTiles` 0x2a570, and the head and tail copies declared
their `CRect`/`RECT` pair at FUNCTION scope, so cl gave each its own group - frame 0xa8 against
retail's 0x5c. Putting a bare `{ }` around each expansion made them siblings and recovered 64
of the 76 surplus bytes (the last 8 came from shifting the screen `Coord` in place instead of
copying into fresh `cx`/`cy` locals). Frame 0xa8 -> 0x60.

related: [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
