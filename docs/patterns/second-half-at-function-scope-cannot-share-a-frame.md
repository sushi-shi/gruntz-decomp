# A guarded first half that `return`s leaves the rest at FUNCTION scope - and its locals stop sharing slots

tags: cpp:branch cpp:local cpp:pointer | asm:sub asm:lea | topic:codegen-idiom
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

A frame-size mismatch is never noise - `gruntz walls diagnose <rva> --asm` and the
uniform `[esp+N]` shift in `gruntz walls diagnose --asm` both point straight at it.
Count the delta in units of `sizeof(the local you suspect)` before reaching for regalloc
explanations.

The same rule applies to a construct expanded N times in one function, which is how it shows
up in the Battlez pathing code: `CMapMgr::Clip` is inlined three times in
`CBattlezMapConfig::RepathAroundBlockedTiles` 0x2a570, and the head and tail copies declared
their `CRect`/`RECT` pair at FUNCTION scope, so cl gave each its own group - frame 0xa8 against
retail's 0x5c. Putting a bare `{ }` around each expansion made them siblings and recovered 64
of the 76 surplus bytes (the last 8 came from shifting the screen `Coord` in place instead of
copying into fresh `cx`/`cy` locals). Frame 0xa8 -> 0x60.

The same block-scope rule applies to an address-taken scalar whose uses form one
contiguous phase. `CProjectile::LoadProjectileSprites` 0xdf050 had a repeated
MFC `Lookup(key, void*& out)` phase followed by unrelated flight calculations.
With `out` at function scope, cl reserved a fresh dword and emitted a 0x20 frame;
putting only the seven lookups and their result stores in one block let cl reuse
the already-dead `a` parameter home and emitted retail's 0x1c frame. Consuming
`m_srcRow = a` and `m_srcCol = b` before the centered-coordinate assignments
also made the two parameter-home scratch slots agree. All eight unwind funclets
became exact and the primary body moved 81.9528 -> 82.1602.

`CGrunt::ResetEntranceAnimation` is the scalar version of the same pattern. Its
tail `CString key` originally sat at `[ebp-0x1c]`; enclosing the key, descriptor,
and final sprite lookup in their natural tail block makes VC5 reuse the dead first
parameter at `[ebp+8]`, exactly matching retail. The primary function bytes and
88.47% score do not change, while the destructor funclet does. Always check EH
metadata after a scope-only edit even when objdiff reports no body movement.

This is not permission to add arbitrary braces around register-only locals. The
retail signature is specific: the candidate has one extra frame dword, a
repeated address-taking call uses that dword, retail instead takes the address
of an incoming parameter home after that parameter's last semantic use, and the
whole phase has a natural lexical end.

related: [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
