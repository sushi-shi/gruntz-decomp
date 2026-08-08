# A shared `const RECT&` local CSEs the address math retail duplicates per inline site

tags: cpp:local cpp:inline cpp:expr | asm:lea asm:add asm:mov | topic:codegen-idiom
symptoms: an if/else where BOTH arms inline the same small helper on the same object; the base is short by a fixed number of bytes and `--diff` shows one arm missing the helper's first two instructions (`mov <reg>,[base+0x48]` / `add <reg>,0x40`) because they were hoisted above the branch
confidence: 9/10

`CGameLevel::PointInRect(const RECT*, x, y)` is inlined at both sites. Its expansion
begins by materialising the rect address and loading `rect->right`. If the argument
is a named reference/pointer local computed BEFORE the branch, cl5 treats that
address arithmetic as loop-invariant-style common subexpression and emits it once
above the branch. Retail emits it inside each arm.

```cpp
// 85.04 - one shared reference; cl hoists `add eax,0x40` + `mov edi,[eax+0x48]`
CGruntzMgr* g = g_gameReg;
const RECT& r = g->m_world->m_level->m_mainPlane->m_viewRect;
if (pick > 0x19) {
    if (CGameLevel::PointInRect(&r, xp, y)) { g->m_cueSink->SpawnVoiceDriver(...); }
} else {
    if (CGameLevel::PointInRect(&r, xp, y)) { g->m_cueSink->LoadGruntSpawnConfig(...); }
}

// 93.62, size now EXACT - the chain is spelled at each site
CGruntzMgr* g = g_gameReg;
if (pick > 0x19) {
    if (CGameLevel::PointInRect(&g->m_world->m_level->m_mainPlane->m_viewRect, xp, y)) { ... }
} else {
    if (CGameLevel::PointInRect(&g->m_world->m_level->m_mainPlane->m_viewRect, xp, y)) { ... }
}
```

cl still CSEs the POINTER CHAIN itself (`g_gameReg -> m_world -> m_level ->
m_mainPlane` is loaded once above the branch, exactly as in retail) - only the final
`&...->m_viewRect` and the first field load move back into the arms. So the fix is
free of duplicated loads and reads as ordinary source.

A `const RECT*` local instead of a reference is identical to the reference (85.01),
and hoisting only `m_mainPlane` into a local is WORSE (84.33) - the whole expression
must be at the call site.

**Second half, same function:** deleting the `CGruntzMgr* g = g_gameReg;` local and
writing `g_gameReg->` at all four uses is worth another 1.7 (93.62 -> 95.34); see
[redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
for why a copy local costs a callee-saved register.

Does NOT apply when the two inline sites are in disjoint branches with no common
dominator load (`CGrunt::LoadVehicleGruntAnimations` 0x63db0, three sites, byte-neutral).

`CGrunt::StepArrivalReroll` 0x00063b60, 85.04 -> **95.38** (size 0x1cf exact; the
residue is a plain ebx/ebp swap between `this` and the `timeGetTime` import pointer).
