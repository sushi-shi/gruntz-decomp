# Positive-form gate (`if (ready) { body }`) is what makes cl shrink-wrap a callee-save push
tags: cpp:branch cpp:return cpp:local | asm:push asm:pop asm:jmp | topic:codegen-idiom
symptoms: retail's prologue saves FEWER callee-saved registers than the recompile and emits the
missing `push esi`/`push edi` after a ready/null gate; retail's early exit jumps to a label BELOW
the corresponding `pop`; recompile pushes all of them up front and the early `return 0` pops all of
them; ~90-93% with the body otherwise byte-exact
confidence: 9/10

[`shrink-wrapped-callee-save-push.md`](shrink-wrapped-callee-save-push.md) files this as a wall.
For the *guard* case it is not one — it is decided by whether the gate is written as an **early
return** or as a **positive-form `if` wrapping the whole body**.

The proof is in retail's own exit structure. `CWarlord::BuildFortSplashParticles` @0x44f80:

```asm
    mov   eax,ds:[g_engineFrameDelta]
    push  ebx
    push  edi                 ; only ebx + edi at entry
    ...
    cmp   dword ptr [eax+0x28],ebx
    je    0x450a2             ; <- the early exit target
    cmp   dword ptr [eax+0x20],ebx
    jne   0x450a2
    mov   eax,dword ptr [edi+0x10]
    push  esi                 ; esi saved ONLY on the path that uses it
    ...
    pop   esi
    or    dword ptr [edi+0x8],0x10000
0x450a2:
    pop   edi                 ; the gate lands HERE - below the pop esi
    xor   eax,eax
    pop   ebx
    ret
```

The gate branches *past* `pop esi`, so `push esi` is provably conditional.

```cpp
// EARLY RETURN - cl saves esi in the prologue, gate pops it: 93.1%
if (sub->m_28 == 0 || sub->m_20 != 0) {
    return 0;
}
/* body */
return 0;

// POSITIVE FORM - cl shrink-wraps push esi past the gate: 98.1%
if (sub->m_28 != 0 && sub->m_20 == 0) {
    /* body */
}
return 0;
```

Same family as [`do-while is an echo`](mfc-map-walk-while-not-guard-dowhile.md): an early return is
an *echo* of a nested block, and the echo is what perturbs the frame. Prefer the positive form
whenever the retail early exit lands below a `pop`.

Two measured cases in one TU, both previously marked `@early-stop` with a "shrink-wrap wall":
- `CWarlord::BuildFortSplashParticles` @0x44f80 — 93.11% → **98.07%** (with the y-before-x
  declaration order that fixes the paired member-load schedule)
- `CWarlord::AdvanceMovingAnim` @0x44e70 — 90.33% → **95.85%**

Residual in both is the unrelated `add eax,0x290` disp8 rebase (see the `@early-stop` note there).

The INVERSE direction in `shrink-wrapped-callee-save-push.md` (retail eager-pushes, recompile
shrink-wraps, `CWormhole::SpawnPartners`) is still a wall — this lever only runs one way.

## Second, independent effect: BLOCK LAYOUT + epilogue tail-merge

The same lever also decides *where the miss handler physically lands*, and that is worth more than
the push. `CInGameText::Update` @0x997c0 has four `return 0` exits. Retail lays them out as

```asm
    ...body...
    <success tail>
    xor eax,eax ; pop edi/esi/ebp/ebx ; add esp,0xc ; ret     ; epilogue A
0x9998f:
    mov  DWORD PTR [ebp+0x58],0xffffffff
    mov  eax,DWORD PTR [ebp+0x38]
    and  DWORD PTR [eax+0x40],0xfffffffe                      ; <- memory RMW
0x9999d:
    pop edi ; pop esi ; pop ebp ; xor eax,eax ; pop ebx ; add esp,0xc ; ret   ; epilogue B
```

The miss handler sits **past** the success return and *falls into* epilogue B, which the three
inner early exits (`jne 0x9999d` / `je 0x9999d`) tail-merge into. Written as an early return
(`if (found == 0) { …; return 0; }`) cl emits that handler INLINE right after the test with its
own 12-instruction epilogue — one basic block too many, and the diff then cascades through every
register assignment downstream. Written as the positive gate with the miss tail after the closing
brace, the layout and the tail-merge both reproduce, and the `&= ~1` even switches from
load/and/store to retail's `and DWORD PTR [mem],imm8` RMW on its own. **79.46% → 93.83%** in one
edit. (Two further shape fixes in the same function — the screen coords loading y before x, and
taking `g_gameReg` into a local *after* both coord loads so cl stops hoisting the singleton —
carried it to 96.59%.)

So: whenever the recompile has an early-exit block cl placed inline and the target has the same
handler parked at the END of the function, that is this pattern, not regalloc.

## The MECHANICAL test — and the bound (measured 2026-07-27, tree-wide sweep)

The lever is **exit-block LAYOUT**, and shrink-wrap is a side effect of it. So the test that
decides whether to apply it is *where the miss handler lives on each side*, not the prologue:

    ret_frac(side) = index of the first `ret` / instruction count      # sema disasm --lite

| base | target | verdict |
|---|---|---|
| low (miss INLINE at top) | high (miss at BOTTOM) | **apply** the positive gate / shared exit |
| both low (miss inline on both sides) | | **already correct** — the prologue delta has another cause |
| high | low | **inverse** — the source must be the *early return*; the positive form is wrong |

Equivalently and more robustly: **count `ret`s on each side.** `b_ret > t_ret` = retail
tail-merges exits our spelling inlines. `b_ret == t_ret` = the gate spelling is already right.

**Applying it where the counts already agree makes things WORSE.** Measured, all reverted:
`CDDrawWorkerHost::CenterScrollA/B` 87.9 -> 72.8, `CGrunt::UpdateGruntStatus` 94.5 -> 77.1,
`CChatBox::ScrollRow0/1` byte-identical (cl canonicalizes both spellings). In all four, retail's
miss handler is INLINE at the top *and* the pushes are still shrink-wrapped — i.e. the two halves
are separable and cl5 will not give you the shrink-wrap from either gate spelling. Those are real
walls; the layout lever is not the fix.

**Apply it in PROPORTION.** `DirectSoundMgr::Play` has base 5 rets / retail 4 — merging *all*
exits (95.5 -> 93.8) over-applied it. Merge as many as retail merges, no more.

## Third form: `goto fail` — the shared-exit spelling for many-early-return functions

A nested positive-form `if` chain is unwritable past ~4 gates. cl emits the **same single bottom
epilogue** for `if (x) goto fail;` … `fail: return 0;`, and this is what retail's many-gate
validators look like (one `xor eax,eax` + one unwind that every gate `jmp`s into):

```cpp
i32 F() {
    T a; U b;                   // declarations hoisted: cl5 rejects a goto that
    if (!p) goto fail;          // skips an initialisation (error C2362)
    ...
    a = ...;                    // assign, don't initialise
    if (!q) goto fail;
    ...
    return 1;
fail:
    return 0;
}
```

Measured on this exact signature (`b_ret` >> `t_ret`):

| function | rets base->retail | before -> after |
|---|---|---|
| `CDDPalette::CaptureSystemPalette` @0x1485b0 | 6 -> 2 | 68.39 -> **100.00 EXACT** |
| `CSBI_GruntMachine::Render` @0xe8cb0 | 2 -> 1 | 88.59 -> **100.00 EXACT** |
| `CDDrawChildGroup::CheckSortOrder` @0x15a780 | 2 -> 1 | 78.70 -> **100.00 EXACT** |
| `HeapCheckDump` @0x118a30 | 1 -> 1 (shrink-wrap half) | 96.70 -> **100.00 EXACT** |
| `CStatusBarMgr::PlaceCursorTarget` @0x105800 | 3 -> 2 | 70.91 -> 97.82 |
| `CFaderShape::ApplyInit` @0x1817e0 (13 gates) | 14 -> 2 | 40.18 -> 72.56 |
| `SoundDevice::CreateBuffer` @0x1366f0 | 7 -> 1 | 35.61 -> 53.52 |
| `CChatBoxOwner::HitTest` @0x21140 | 8 -> 3 | 38.11 -> 63.06 |
| `SoundStream::CreateStreamBuffer` @0x137780 | 7 -> 1 | 40.70 -> 54.73 |
| `CRezItm::Close` @0x13c830 | 3 -> 2 | 81.28 -> 93.08 |
| `CSBI_WarlordHead::Render` @0xeb880 | 2 -> 1 | 87.72 -> 95.45 |
| `CCheckpointTriggerSwitchLogic::BuildSmall` @0x112a50 | 6 -> 4 | 62.39 -> 72.55 |
| `CLatencyList::FillCombo` @0x37ff0 | 2 -> 1 | 63.45 -> 72.01 |
| `CMapMgr::UpdateDiagonals` @0x82030 | 2 -> 1 | 52.13 -> 54.43 |
| `CFontConfig::RenderInputText` @0x22160 | 2 -> 1 | 75.22 -> 74.22 (layout right, other residue) |

**Does NOT apply when the base is missing structure the target has.** Screened but rejected
because the base lacks retail's `/GX` frame entirely (a destructible-local modelling gap, not a
layout choice): `RebuildPlanes` @0x1628f0, `InstallTree` @0x154f80, `FillCustomLevelList` @0x3af90,
`CWwdGameObject::CreateObject` @0x166640, the `CDDrawChildGroup::CreateObject_*` family.
**Also does not apply** to `CStatusBarMgr::BuildGameMenu` @0x101580 (11 -> 2 rets): the `goto fail`
form DID collapse the exits (11 -> 3) but cl then dropped the `/GX` frame the `new`-expression
cleanup requires, 58.97 -> 49.36. Reverted. A /GX function whose shared-exit block itself runs a
destructor keeps the per-site spelling.

related: shrink-wrapped-callee-save-push.md, mfc-map-walk-while-not-guard-dowhile.md,
retry-loop-bail-while-goto-no-peel.md, identical-return-epilogue-tailmerge.md
