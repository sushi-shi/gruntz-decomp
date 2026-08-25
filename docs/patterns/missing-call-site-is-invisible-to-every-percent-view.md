# A MISSING call site is invisible to every % view — diff the per-function CALL COUNT

- **Confidence**: 10/10
- **Tags**: `cpp:call` `cpp:virtual` | `asm:call` | `topic:correctness` `topic:tooling`

## Symptom

The function scores in the 80s-90s, `--diff` and `gruntz walls diagnose --asm` report the
surrounding code as matching, `gruntz walls diagnose` reports the branch sequence as
equal, and the game still faults — or silently never runs an initializer. Nothing
in the matching toolchain points at the hole, because every % view scores the
instructions that ARE there and objdiff masks address operands.

The **virtual** form is the worst case: `call dword ptr [edx + 0xa8]` carries no
relocation at all, so `insn_seq --seq` (reloc-carrying instructions only) cannot
see it either.

## Mechanism

Measured 2026-08-10 on `CPlay::LoadByMode` (0xca200). Retail:

```
243e: 8b 16                 mov  edx, dword ptr [esi]     ; vptr
2440: 55                    push ebp                      ; reload
2441: 8b ce                 mov  ecx, esi                 ; this
2443: ff 92 a8 00 00 00     call dword ptr [edx + 0xa8]   ; CPlay::BuildWorldLevelPath
2449: 85 c0                 test eax, eax
244b: 0f 84 ec 05 00 00     je   <fail0>
```

Our source had no such call anywhere: `CPlay::BuildWorldLevelPath` was DEFINED
(`src/Gruntz/PlayAssetLoad.cpp`, and overridden by `CDemo`) and reachable only
through the vtable, so nothing referenced it and no gate noticed. That method IS
the level load (`m_world->m_level->LoadFromSource/LoadFromFile` →
`CGameLevel::LoadWwd` → `ReadPlane`, the only writer of `m_mainPlane`). Without
it, `LoadByMode` ran the whole asset chain over an unloaded level and faulted at
+0x84e on `m_mainPlane->m_gridW` with `m_mainPlane == NULL`. `LoadByMode` scored
83.84% with the call absent.

## Detection

`build/callcount_detector.py` (candidate for `gruntz.audit`): disassemble every
objdiff base obj and every delinked target obj, split at COFF symbol headers, and
diff the per-function call counts, bucketed `direct` / `indirect` / `import` /
`tail`.

    python3 build/callcount_detector.py              # ranked worklist
    python3 build/callcount_detector.py --slots      # VTABLE-SLOT multiset diff
    python3 build/callcount_detector.py --unit play

`--slots` is the virtual-call view: it reports, per function, which
`call dword ptr [reg + 0xNN]` slot offsets retail dispatches on and we do not.
Exclude `esp`/`ebp` displacements — those are stack fn-ptr slots whose offset
tracks the FRAME SIZE, and comparing them yields nothing but frame-size noise.

The complementary whole-program view is a reference census: a symbol that retail
`.text` references and our `.text` never does at all. It does NOT catch the
virtual case (the vtable still references the method on both sides), so run both.

## Adjudication — most rows are NOT missing calls

A count deficit has three causes and only one is a defect:

1. **cl cross-jumped two ARGUMENT-IDENTICAL call sites that retail kept apart.**
   This is the dominant class. `CGrunt::UpdateArrival` shows retail with one more
   each of `GruntInRadius`, `RectContains`, `ResetEntranceAnimation` and
   `PlayVoice`; the source has every one of those sites, textually
   identical in two arms, and cl merged each pair. Same for
   `CWwdSpatialMgr::Relocate` (four `m_mgr->RemoveAll(cur, obj)` sites → one),
   `CCheatMgr::LoadCheatConfig`, `CFaderShape::RenderFrame`,
   `CPlay::ValidateLevelTiles`, `CInGameIcon::CInGameIcon` (seven `SetupSprite`).
   Behaviourally identical; do not "restore" anything.
2. **Inline-budget divergence** — retail calls a header inline we expand, or the
   reverse (`CUserLogic::AttachToObject` in the logic ctors, `CMenuItem::Reset`
   in `CMenuPage::AddSubItem2`, `zPTree::Reset` in
   `CChatBoxOwner::HandleTextInputKey`, the `CFecFile`/`CArray` COMDATs in
   `CGruntzMgr::PlayMovieEntry`). Also benign.
3. **A genuinely missing call** — the class above. Rare, and worth the whole
   sweep.

So: read the CALLEE NAMES, not the count. If every retail-only callee already
appears in the source for that function, it is (1) or (2). If a callee appears
in retail and NOWHERE in our source for that function, it is (3).

## Related

`docs/patterns/masked-diff-hides-branch-target.md` (the same blindness, one level
down), `[[hundred-percent-can-still-be-wrong]]`.
