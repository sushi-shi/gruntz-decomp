# Reloc-typing: vptr/global/IAT store operands differ (code bytes match) — scoring artifact
tags: cpp:ctor cpp:member | asm:mov asm:call | topic:scoring-artifact topic:tooling
symptoms: ~99.5% fuzzy with 0 structural diffs, diffs only in 4-byte operand slots, ??_7…/__imp__/DAT_ names
confidence: 9/10

**CORRECTION (2026-07-01, directly MEASURED — SUPERSEDES the "name a DIR32 data referent to score
exact" claim in the 2026-07 note below):** DIR32 data-reloc **NAMES do NOT gate exactness** under
the current objdiff config — naming a global (`DATA()`), a pooled `??_C@` string (`coff_oracle`),
OR a `??_7` vtable (`vtable_names.csv`) flips **NOTHING**. Three independent proofs:
1. **1684** DIR32 name-mismatches sit inside functions already scored **100.0% exact** — including
   **7 pooled `??_C@` string literals** (`RegistryHelper::Open` is 100% with base `??_C@_08LOIE@Software`
   vs target `s_ware_0061a068`; `CWorldState::BuildWorldLevelPath` 100% with `??_C@…BATTLEZ` vs
   `s_BATTLEZ__006130b0`) and **205 in-TU-emitted `??_7` vtables** (`CGameWnd::CGameWnd` is 100% with
   base `??_7CGameWnd@@6B@` vs target `?g_vtbl_5ea344@@3PAXA`). If any of these names gated exactness,
   those functions could not be 100%.
2. **Controlled test:** renaming a near-miss's SOLE unnamed DIR32 referent so base==target left the
   fuzzy% **byte-identical** — `GruntzPlayer::Serialize` 98.13333%→**98.13333%** after
   `g_serialCount`→`g_serialCounter`; `CDDrawSubMgrLeafScan::CreateEntry`/`CreateEntry2`
   **99.809525% unchanged** after DATA-binding their sole `g_leafElemVtbl` referent (relocdiff
   confirmed the DIR32 mismatch vanished; the score did not move).
3. **High-fan-out target-rename (2026-07-04):** the game-mgr singleton at RVA `0x24556c` is
   referenced by ~45 base objs as `?g_gameReg@@3PAUCGameRegistry@@A` (C++), by ~48 as extern-C
   `_g_mgrSettings`, and by ~23 as other-typed `?g_gameReg@@3PAU<T>@@A`; the delink target names it
   `_g_mgrSettings` (labels.py DATA keep-last, `_`>`?` sort). Pinning the target's `0x24556c` name to
   the base C++ symbol via a labels.py canonical-override + full rebuild (delink + objdiff re-ran,
   529/529) left **every** metric byte-identical — overall **1861 exact / 68.75% fuzzy → 1861 /
   68.75%**, every per-unit % and every sampled per-function fuzzy% bit-for-bit unchanged.
   `CBattlezData::FillRecord` is **100.0% (a counted match)** with base `?g_gameReg…` vs target
   `_g_mgrSettings`. So a both-named-but-differently-named DIR32 data referent is fully masked: do
   **NOT** chase `g_gameReg`/`g_mgrSettings` name alignment (source rename OR a synth-PDB alias) as a
   scoring lever — it is a proven 0-delta no-op.

objdiff pairs DIR32 relocs by reloc TYPE + addend (and, for defined data, by pointed-to CONTENT),
NOT by symbol name. Consequence for the near-100% pool: a code-byte-exact near-miss is capped by a
**REAL codegen diff** (regalloc/scheduling/EH-state/instruction-selection) or by a string/vtable
**SHAPE** diff (a named-static `char*` vs a pooled literal emits different LOAD-vs-PUSH *code* — a
body-shape reconstruction, not a name), NEVER by an unnamed data referent's name. **Do NOT run
name-only "flip" passes on the near-100% pool** — they yield 0 flips (measured: 7 TUs of global
unification → 0 exact delta; this pass → 0 exact delta). The `REL32`/import half of the older note
stands (those are masked too). See `docs/matching-patterns.md` §215 and `docs/wall-instructions.md`.

**UPDATE (2026-07, measured — largely OBSOLETE; its DIR32 half is now REFUTED, see CORRECTION above):**
The vtable/global **DIR32** half is
FIXED — a NAMED DIR32 data referent (vtable via `config/vtable_names.csv`, global via `DATA()`,
pooled string via the `coff_oracle`) now scores exact; an unnamed one is fixed by naming it. The
**`__imp__`/import + `REL32`-call half is a NON-issue**: objdiff MASKS `REL32` call/branch reloc
names and `call [disp32]` import operands, so those never cap the score (measured: renaming 5909
call/thunk relocs + adding 788 `__imp__` relocs → 0.0% delta). Net: this is **no longer a standing
wall**. A code-byte-exact function that still caps is an UNNAMED DIR32 data referent (→ name it) or
a REAL codegen diff — never a call/import/thunk-name artifact. See `docs/matching-patterns.md` §215
and `docs/wall-instructions.md`.

A function that stores a vtable pointer (`mov [this], offset ??_7Class@@6B@`), a global, or
calls an import (`FF15 [IAT]`) plateaus at ~99.5% **fuzzy** but is byte-exact in CODE. The
delinker emits the target operand against a differently-named symbol (`DAT_*`/`__imp__*`/the
Ghidra name) than `cl`'s real symbol, so the 4-byte operand slot mismatches even though the
instruction bytes are identical. CONFIRM by `llvm-objdump -dr` base vs target: the only diffs
are reloc operand slots. Then it is MATCHED (orchestration §6/§8) — do not chase the phantom.

WALL (scoring artifact, code already correct). Our analog of vostok's
`delinker-masks-base-immediates`. Evidence: every ctor/error-formatter; CDirectDrawMgr 96.2% (10 reloc slots).
