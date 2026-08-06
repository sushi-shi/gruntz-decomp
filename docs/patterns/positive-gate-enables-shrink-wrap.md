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

## The four SPELLINGS the ret-count test resolves to (second sweep, 2026-07-27)

`b_ret > t_ret` says *an exit block is duplicated*; it does not say which spelling fixes it.
Measured over 21 functions (20 improved, 1 EXACT), the extra block is always one of four
shapes, and each has its own rewrite. Read the retail block that the counts point at before
picking one — the wrong one of these four can cost as much as the right one gains.

### 1. Search loop — hoist the guard, write `if (c) { do {…} while (c); }`

A `for`/`while` search whose fall-through is the miss `return 0` makes cl place a **second**
miss epilogue right after the loop and take the back-edge conditionally. Retail instead
branches **out** of the bottom test into the shared miss block and makes the back-edge an
**unconditional `jmp`**:

```asm
; retail                                   ; base
  cmp  edi,eax                               cmp  edi,eax
  jge  <shared miss>                         jl   <loop top>
  jmp  <loop top>                            xor  eax,eax   ; <- the duplicate
                                             pop/pop/ret
```

Rewrite the loop with its guard hoisted (`i32 i = 0; if (i < n()) { do { … i++; } while (i < n()); }`).
Folding the *other* `return 0`s into one statement is **not** enough on its own — measured:
`CPlay::FindStartPointAt` did not move at all until the do-while went in.

| function | rets | before → after |
|---|---|---|
| `MgrListFind` @0xf0db0 | 3 → 2 | 73.22 → **89.47** |
| `CPlay::FindStartPointAt` @0xd5f90 | 3 → 2 | 83.22 → **93.49** |
| `CTriggerMgr::CellHitTest` @0x6bea0 | 3 → 2 | 71.17 → **80.71** |
| `CTriggerMgr::FindGruntAt` @0x75c60 | 3 → 2 | 74.75 → **79.42** |

`MgrListFind` and `CellHitTest` had been filed as *regalloc* walls; the third callee-saved
register fell out on its own once the layout was right.

### 2. Window/dialog procs — `break`, not `return 0`, in a case arm

When a `switch` arm's "not handled" exit is spelled `return 0` it gets its own epilogue AND
cl emits the switch **default** inline in the compare ladder (`je <last case>`; default falls
through). Make that arm `break` so it lands on the switch's trailing `return 0`: the default
block moves to the bottom, the ladder's last compare **inverts** to `jne <default>`, and the
last case becomes the ladder fall-through — retail's exact layout.

| function | rets | before → after |
|---|---|---|
| `CGruntzWnd::PreDispatchMessage` @0x94790 | 6 → 3 | 77.64 → **98.19** |
| `LevelPreviewDlgProc` @0xe3690 | 4 → 1 | 80.35 → **85.15** |
| `GameOptionsDlgProc` @0x36410 | 12 → 10 | 96.05 → **97.64** |

**And the dispatch has to actually BE a `switch`.** Chained `if (msg == X)` lowers to
`cmp/jne` pairs; retail's `sub eax,0x110 / je / dec eax / jne` (and `dec eax / je / dec eax
/ je` on wParam) is a switch. `ButeAttributezDlgProc` @0x3c990 needed **both** halves —
merging the exits alone took it 27.94 → **14.94**, and adding the two switches took it to
**100.00 EXACT**. Do not judge half of this fix by itself.

### 3. Many gates — carry the value in a local, don't `goto fail`

`goto fail; … fail: return 0;` still leaves cl laying the fail block *before* the success
tail, i.e. two epilogues. What reproduces retail's single epilogue is carrying the result:

```cpp
i32 ok = 0;
if (!Step1()) goto done;
…
ok = 1;
done:
return ok;          // the success tail FALLS INTO the one epilogue
```

`CBootyState::LoadGameAssetNamespaces` @0x18830, 2 rets → 1, 86.20 → **88.48**; retail's last
five gates then need no `xor eax,eax` at all because eax is already 0.
**Not universal:** the identical rewrite on `LevelPreviewDlgProc` regressed 85.15 → **79.79**
(reverted) — a second lane had already measured the same thing there. Try it, keep the number.

### 4. Two-arm store at a tail — retail selects the SLOT and stores once

`if (d) p->a = v; else p->b = v;` immediately before a return gives cl two stores each with
its own epilogue. Retail computes the destination and stores once
(`lea eax,[esi+4]` / `je` / `mov eax,esi` / `mov [eax],…`):

```cpp
T** slot = &p->m_child[1];
if (d) slot = &p->m_child[0];
*slot = v;
```

`zPTree::Insert` @0x16db90 5→4 rets, 53.39 → **60.47**; `CProjActMap::Insert` @0x1933b0 4→3,
58.21 → **63.58** (its `m_0`/`m_4` are the real `m_child[2]` array retail indexes).
`CWarpStoneFly::Tick` @0x10a0f0 is the same shape with a `goto` (both y arms share one clamp
store), 79.37 → **80.52**.

### 5. Two-arm CALL — retail duplicates the whole call, cl selects the argument

The mirror of (4). When both arms make the *same* call with a different argument, cl will
happily compute the argument into one register and emit the call once; retail emits the
call in **both** arms and lets cl tail-merge everything after the differing `push`:

```asm
; retail                              ; recompile
  test eax,eax                          test eax,eax
  je   ELSE                             lea  eax,[esp+0x10]
  lea  ecx,[esp+0x10]                   jne  JOIN
  push ecx                              mov  eax,[esi+0x34]
  jmp  JOIN                           JOIN:
ELSE:                                   push eax
  mov  edx,[esi+0x34]                   push 0 ; push 0x180 ; push edi ; call ebx
  push edx
JOIN:
  push 0 ; push 0x180 ; push edi ; call ebx
```

The tell is retail's **`push X` / `jmp` in one arm and `push Y` in the other**, with the rest
of the argument list and the `call` shared — that is cl's own tail-merge of two complete
calls, not a select. So write two calls, not `const char* s = c ? a : b;`:

```cpp
if (NetFormatKeyed(buf + 4, p->m_desc.m_lpszName, "NAME")) {
    idx = SendMessageA(h, LB_ADDSTRING, 0, (LPARAM)buf);
} else {
    idx = SendMessageA(h, LB_ADDSTRING, 0, (LPARAM)p->m_desc.m_lpszName);
}
```

`FillPlayerList` @0xb89e0 92.72 -> **95.41** (2026-07-28); the selected-pointer spelling also
inverted the guard, which is how `jcc_sieve` surfaced it.

**Cost check first.** Duplicating the call duplicates every cast in the argument list. In
`FillPlayerList` that is a second `reinterpret_cast<LPARAM>`, which trips the
`reinterpret_casts` ratchet (440 -> 441, FATAL) - so that one was reverted and the shape is
recorded in its `@early-stop` instead. Apply this where the duplicated argument needs no
cast; there the shape is free.

### Also: a mid-function guard that duplicates a LATER shared exit

`if (!CanWrap()) return 0;` sitting above a `if (!found) return 0;` is a duplicate of it —
retail branches the first straight into the second. Writing the gate positively
(`if (CanWrap()) { …wrap scan… }`) merges them: `CMenuPage::FocusNext`/`FocusPrev` @0x183c50/
0x183d10, 8→7 rets, **77.13 → 97.18 each** (both were filed as regalloc walls).
Same family: `CMulti::WaitForOtherPlayers` @0xbb700 4→3, 75.04 → **80.47**;
`CMulti::VerifyCustomLevel` @0xb8fc0 6→1, 6.20 → **23.46**;
`FindProcessByName` @0x118ce0 3→2, 86.14 → **93.54**;
`CStatusBarMgr::ActivateSlot` @0x10b930 6→4, 71.11 → **83.63**;
`CGruntVoice::Update` @0x11a8e0 6→4, 73.24 → **85.84**;
`WapUncompress` @0x1853b0 3→2, 85.93 → **87.59**;
`CGameLevel::MoveHandlerA` @0x15e130 4→3, 64.96 → **70.04** (that one was a genuine
control-flow bug the count exposed: retail runs the held-flag tail on the probe MISS of both
arms, so it is not an `else`).

### The INVERSE direction (`b_ret < t_ret`) is a WALL — do not spend a session on it

Everything above is for `b_ret > t_ret` (we duplicate an exit retail merges). The mirror —
**retail has MORE epilogues than we do** — is not source-steerable, because cl5 tail-merges
identical epilogues regardless of where the source puts the block.

`CProjectile::ScanTargets` @0xe0b10 (1 -> 2 rets) is the worked case. Retail lays out
`[loop][epilogue A][self-cell handler][epilogue B]` — the row loop's fall-out gets its own
epilogue A (so the back-edge is a plain `cmp/jl <top>`), the cold self-cell handler is sunk
*past* it, and the hit-list `return` shares B with the handler. We get
`[loop][handler][one shared epilogue]`, which forces the loop exit to jump over the handler
(`jge <shared> / jmp <top>`) — one block and one `ret` too few, and it is 1 byte SMALLER, so
cl has no reason to prefer retail's form. Three spellings measured 2026-07-28, all
**byte-identical** at 93.99:

1. the handler written inline in the loop with `return;`
2. the handler sunk to a `return; selfcell:` tail below the function's own `return`
   (hoisting `g` to function scope so the tail can still read it)
3. (2) plus the hit-list `return` routed to a `done:` label placed *after* the handler —
   i.e. retail's exact edge structure spelled out

Same wall as `EngStr_DrawText` @0x115440 / `ShowHudMessage(Alt)` @0x1154b0/0x115520 (1 -> 2
rets), where a previous lane enumerated `if(!cfg)return` / `==0` / `else` and none split the
bare `void` ret — see [identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md).

So: when `jcc_sieve` shows `rets N -> N+1`, read it as *diagnosed, not actionable*, and spend
the budget on the function's other residue.

### Bound, re-confirmed

`b_ret > t_ret` can also mean **the base is missing a whole inlined construction**, not a
layout choice. `CDDrawWorkerRegistry::DispatchKeyed{2C,30,34,38}` @0x156xxx read 2 → 1, but
retail inlines `operator new(0x6c)` + the field stores + an inner ctor under a `/GX` frame
where the base just calls a factory. No layout edit can close that; screen it out by looking
for the EH prologue the base does not have.

## The inverse is REAL: a measured counter-instance (2026-08-01)

The rule above is not symmetric and must not be applied blind. `SoundDevice::FreeSamples`
@0x136ed0 is the counter-instance: **retail does NOT shrink-wrap and we DO**, from the early-
return form on both sides.

```asm
; retail: all four saved in the prologue, the gate pops all four
    mov   eax,[ecx+0x78]
    push  ebx
    push  ebp
    push  esi
    test  eax,eax
    push  edi
    jne   0x136ee2
    xor   eax,eax
    pop   edi / pop esi / pop ebp / pop ebx
    ret
```

The loop body is **byte-identical** (including the `neg/sbb/and` null-mask landing in eax);
only the prologue placement differs. A 4-cell matrix over the guard refutes the rule
here:

| spelling | score |
|---|---|
| `if (m_initialized == 0) return 0;` (early return) | **77.31** |
| `if (!m_initialized) return 0;` | 77.31 |
| `if (m_initialized) { loop; return 1; } return 0;` (positive) | 72.13 |
| `i32 ok = 0; if (m_initialized) { loop; ok = 1; } return ok;` | 71.39 |

The positive form *costs* 5 points. And retail's own polarity is the early return
(`jne body`, the `return 0` as the fallthrough), so both sides already agree on the gate — the
gate was never the variable.

**The screen.** Its sibling `DSoundCloneInst::GetItem` @0x135d70 needs the OPPOSITE shrink-wrap
decision (retail saves only `edi` at entry and defers `push esi`/`push ebx` past the guard,
where cl pushes all three up front) from the *same* guard shape, and all four of its guard
spellings tie at 90.31. Two functions in one TU, one guard shape, opposite required outcomes
⇒ the decision is the allocator's, not the source's.

So before reaching for the positive form, check that retail's exit actually lands **below** a
`pop` (the proof quoted at the top of this doc). If retail's early exit pops *everything*, the
rule does not apply and the positive form will cost you points.
