# `goto fail;` is the ONLY spelling that shares SOME exits and duplicates the rest
tags: cpp:branch cpp:goto cpp:return | asm:xor asm:jcc asm:ret | topic:codegen-idiom
symptoms: base has MORE ret blocks than target, DUP-EXIT, retail's leading guards `je` a shared exit while ours fall through to their own copy, missing `xor eax,eax` in a guard exit, elided xor, cross-jump, tail merge, exit block placement
confidence: 9/10

cl 5.0 /O2 has exactly **three** exit-merging regimes, selected by the SOURCE
construct that reaches `return 0;`. Nothing else moves them - not `/GX`, not body
size, not jump distance, not statement order.

| source | result |
|---|---|
| separate `if (c) return 0;` | **no merging.** One inline copy per site, each the fall-through of its own inverted `jcc`. |
| `goto fail;` … `fail: return 0;` | **partial.** The goto sites share ONE block; every OTHER `return 0` in the function keeps its own copy. |
| `\|\|`, `&&`, a positive-gate nest, or `if (a) goto L; if (b) goto ok; L:` | **total.** EVERY same-valued `return` in the whole function collapses into ONE block, sunk past the last instruction. |
| `if (X) { ... return K; } else { return K; }` | **total** - a fourth spelling of the row above, byte-for-byte identical to `\|\|`, and the one that does NOT grep as one. See [if-else-both-arms-return-is-the-or-regime.md](if-else-both-arms-return-is-the-or-regime.md). |

Retail's guarded `Init`/`Build`/`Setup` functions are the *partial* regime, so
transcribing them as separate `if`s leaves us with 2-4 surplus `ret` blocks, and
"fixing" it with `||` overshoots to one. `goto fail` is the only spelling that lands
in between. Where the shared block ENDS UP is also mechanical: cl inverts the LAST
goto's branch and makes the shared block that branch's fall-through, so the guard you
send to `fail` last decides its position.

```cpp
// NO - six ret blocks; retail has four
if (host == NULL)  return 0;
if (owner == NULL) return 0;
...
if (key == NULL)   return 0;
...
if (tbl == NULL)   return 0;

// NO - one ret block; the || swallows the key guard too (88.49 -> 81.75)
if (host == NULL || owner == NULL) return 0;

// YES - the leading pair and the LAST guard share one block, `key` keeps its copy
CObject* found;                 // hoist: a goto may not skip an initialisation (C2362)
CDDrawWorker* tbl;
if (host == NULL)  goto fail;
if (owner == NULL) goto fail;
...
if (key == NULL)   return 0;    // deliberately NOT a goto - retail duplicates here
...
if (tbl == NULL)   goto fail;
...
return cel != NULL;
fail:
    return 0;
```

**The `xor eax,eax` elision is a CONSEQUENCE, not the cause.** cl drops the `xor` from
an exit block that has ONE predecessor and reaches it with the tested value already in
`eax` (`if (p == NULL) return 0;` on a value in eax lowers to `pop esi; ret`). That
makes the copy 2 bytes, which is what tips cl into duplicating rather than jumping - so
the elision and the duplication are the same decision seen twice. Give the block a
second predecessor with `goto fail` and the `xor` comes back on its own; there is no
separate lever for it, and none of `int z = 0; return z;`, `(BOOL)0`, `FALSE`, `NULL`,
a `volatile` local, a named result local, `== 0` instead of `!`, or an intervening
store changes it (all seven measured on a `if (!m_p) goto fail; if (!m_p->IsLoaded())
goto fail; ... fail: return 0;` probe).

Screen: `gruntz walls diagnose <rva>` (base ret count > target's).

Measured 2026-08-08:
`CSBI_ImageSetAni::Init` 0xe7980 88.49 -> 96.43 (rets 6->4, exits byte-exact),
`CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar` 0xea1f0 88.56 -> 94.68 (10->8),
`CSBI_GruntMachine::BuildResourceTabStatusBar` 0xe8a70 85.91 -> 93.17 (7->5),
`CDDrawSubMgrPages::TransEnter` 0x158e40 88.21 -> 97.31 (6->5, one `xor` left).
`CGameInfo::SetNames` 0x118040 already used this spelling and is EXACT - it is where
the recipe was found, not invented.

## Read WHERE retail's shared block sits - that is what picks `goto` vs `||`

The two merging spellings differ in PLACEMENT as well as in reach, and the placement is
the cheaper thing to read off the target:

| retail's shared exit sits | spelling |
|---|---|
| between the guards and the rest of the body | `goto fail;` - cl inverts the LAST goto and gives it the fall-through |
| sunk PAST the whole body, after the success return | `\|\|` - the total regime |

`CPlay::OnLButtonDblClk` 0xce660 is the worked example: two adjacent `return 0` guards
that retail merges into a block at the very END. `goto fail;` merged them but parked the
block mid-function (86.53 -> 90.35, rets 10=10 but one branch polarity wrong); the same
two guards as `if (a || b) return 0;` put it where retail has it (-> **95.70**, branch
sequences agree). **The label's position in the SOURCE is irrelevant** - moving
`StepArrivalDefense`'s `seek:` label from function scope into the end of the `case` block
it is emitted in was byte-identical.

Equal return counts do not rule this out. `CSBI_SideTab::BuildStatzTabStatusBar`
0xe9600 had two returns and the same three incoming failure edges on both sides, but the
source's leading `if (host == NULL || parent == NULL)` selected the total regime and sank
the shared zero epilogue past the success return. Retail placed that epilogue immediately
before the success block. A trailing `fail:` label alone was byte-identical; only splitting
the short-circuit into two separate `goto fail` guards selected the partial regime and
changed the last `je` to retail's `jne`. With the two arm-local member assignments already
in retail order, this closed **90.0746 -> 100.0000 EXACT**: 0x18c bytes, 134 instructions,
16 branches, two returns, and seven ordered referents on both sides. Detection therefore
uses block position and predecessor identity, not merely a return-count delta.

## The shared block does not have to be a bare `return`

Any statement list that several guards jump to is one label. `CGrunt::StepArrivalDefense`
0xf2b20 had FIVE copies of `{ m_defenderState = AISTATE_SEEK; return 1; }` against
retail's one; a `seek:` label took it 76.85 -> **83.09** (rets 16->13, target 12). Its
near-clone `CGrunt::StepArrivalDefenseLean` 0xf8240 took the same edit 54.22 -> **63.47**
(rets 13->10, matching retail).

## In an EH (`/GX`) function, distinguish cleanup-call sites from full epilogues

Repeated destructor CALL sites are still source-flow evidence. The two
`CVoiceManager::PlayVoice` overloads each began with two emitted
`CString::~CString` calls against retail's seven because a positive-gate
`SetSource(...) && Configure(...)` collapsed every failure onto one cleanup.
Making all failures separate overshot to eight calls. Sending only the
`SetSource` and `Configure` failures to one `streamFailed:` label produced the
retail census exactly: 21 calls, 7 CString destructors, and the same branch and
relocation counts. The pointer overload moved 76.41 -> **90.10** and the object-id
overload 78.34 -> **92.47**; both then diagnosed as regalloc/scheduling with equal
EH action sequences and zero referent divergence. In this shape, count the
destructor calls in the parent function before dismissing the residue as EH
epilogue scheduling.

The census can expose missing behavior before it exposes a spelling. In
`CVoiceManager::PlayGruntVoiceCue` 0x11afb0, retail calls `SetSource`,
conditionally calls `Configure`, tests the surviving return value, and reaches
`CGruntVoice::BeginPlayback` only when both calls succeeded. The reconstruction called
`Configure` conditionally but ignored both failure results and always ran
`BeginPlayback`. Restoring the positive `SetSource(...) && Configure(...)` gate added
the missing branch and moved 91.23 -> 91.78 (82.23 before the preceding
structural repairs). Only then was the remaining 4-vs-6 CString-destructor
census safe to classify as cleanup merging. A two-site failure label and a
complete post-CString `goto fail` regime were both byte-identical to the
compound gate and did not restore the extra cleanup pair. Inspect the
instructions immediately before a repeated-destructor delta: a missing result
gate is a semantic defect; only the residual duplicated cleanup is a wall.

When the duplicated blocks are instead the whole unwind epilogue, they are an
epilogue cross-jump and not this lever.

When the duplicated blocks are the whole `mov ecx,[esp+N]; pop...; mov fs:0,ecx; pop ebx;
add esp,N; ret` unwind epilogue, no source construct moves them. Retail emits ONE epilogue
and `xor eax,eax; jmp <it>` at each failure site; we emit a full copy per exit. Measured on
`CLatencyList::FillCombo` 0x37ff0 (2 rets vs retail's 1): a trailing `fail:` label, three
plain `return 0;`s, a single `i32 n; ... done: return n;` result variable, and flattening
the nested `if (combo != NULL) {...} return 0;` into a flat guard were ALL tried - the flat
and single-variable forms are strictly worse (3 rets, 72.01 -> 63.45) and the rest are
byte-identical. The cause is visible in the two base epilogues: cl schedules `mov fs:0,ecx`
*before* the pops on the path with no spare instruction and *after* them on the path that
has a `xor eax,eax` to fill with, so the two epilogues are not identical and the cross-jumper
declines. Retail's single epilogue is the second ordering minus the `xor`.

It is not "our cl never shares EH epilogues" either - the OVER-MERGE direction shows the
same coin landing the other way (`CButeMgr::SetInt` 0x171b80: we emit 1 epilogue, retail 5).
Treat this family as a scheduling wall and skip it: 0x18830, 0x3f5f0, 0x37ff0, 0x22160,
0x101580, 0xffde0, 0x10a340, 0x102250, 0x156ad0.

## DUP-EXIT is also a correctness-bug detector

Two of the biggest wins in the 2026-08-08 sweep were not spelling at all - the surplus ret
was a real reconstruction bug the ret count made visible:
`CTriggerMgr::DestroyGroup` 0x798d0 had `return 0;` *outside* the `LoadAssets() == 0` block,
so it bailed even when the overlay was built (86.63 -> **100.00 EXACT**, with a
`RECT* vr = &view->m_mainPlane->m_viewRect;` for the last two loads);
`CGrunt::OnStruck` 0x588f0 had `m_struckCount = 0;` in an `else` where retail runs it
unconditionally after the `if` (75.48 -> **90.39**). Check the guard's semantics against the
target blocks before reaching for a label.

**The mirror direction is NOT solved when our side spells the TOTAL regime.** When
retail has MORE exits than we do (`exit_merge_sieve --over`, 47 functions) and our
source has a `||`/`&&` guard collapsing the others, there is no spelling that keeps
one solo `return 0` while a *sunk* shared block exists: the goto form always hoists
its block in front of the body instead of past the success return. Measured on
`CSBI_MenuItem::SetupImage` 0xe80e0 (92.17 -> 84.24 with goto, reverted) and
`CDDrawSurfacePair::InitFromSurface` 0x163db0 (77.50 -> 56.74, reverted).

**But when our side spells the PARTIAL regime it IS solved - drop to NONE.** The
other half of the mirror is a source that already says `goto fail;` where retail
duplicated the exit at every guard. Its signature is not a ret-count delta at all
(both sides have one `ret`): each of retail's failed steps reads

```asm
jne  <next step>            ; base has `je <the shared fail block>` instead
xor  eax,eax
jmp  <the epilogue>
```

and, because a duplicated exit carries no EH-state join, retail's `/GX` state
numbers at the *following* guards are all one higher than ours - so the immediates
in `mov [esp+N],<state>` diverge for the whole rest of the body and the wall reads
much larger than the exit blocks themselves. Replacing every `goto fail;` with a
plain `return 0;` and deleting the label is the whole fix:
`CBootyState::LoadGameAssetNamespaces` 0x18830 92.52 -> **99.53** (six duplicated
exits recovered, branches 17 -> 22 against retail's 22). Read which regime the
SOURCE currently spells before concluding the mirror is unreachable.

`/Os` and `/O1` DO enable a real machine-level cross-jump pass (a 6-ret probe collapses
to 1), so the pass exists - but it is all-or-nothing and everything else about the
codegen changes: `#pragma optimize("s", on)` around `CSBI_ImageSetAni::Init` scored
88.49 -> 45.07. Not a lever.

related: [trailing-error-block-is-a-crossjump-magnet.md](trailing-error-block-is-a-crossjump-magnet.md) (the wall this replaces), [one-shared-return-tail-is-a-positive-gate-nest.md](one-shared-return-tail-is-a-positive-gate-nest.md) (the TOTAL-merge regime, when retail really has one tail), [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md) (the prologue half of the same layout decision)
