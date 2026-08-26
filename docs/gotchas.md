# Gotchas — measurement, build, and matching traps

Hard-won traps that cost real time. Grouped by area. The deeper codegen idioms live in
`docs/patterns/`; this file is the fast index of the *surprises*.

## Measuring match % (objdiff)

- **`objdiff --diff` MASKS large immediates as `<addr>`.** A wrong magic constant (a
  reciprocal divisor `0x88888889`=/30, `0x2aaaaaab`=/12, `0x55555556`=/3; an address; a
  flag word) is INVISIBLE in the plain diff and shows only as a downstream `sar`/`shr`/`lea`
  shift. **Always verify constants with `--base`** (the raw disasm). Multiple exact matches
  hid behind this (`/9`-vs-`/30`, FP `fild` operands).
- **`objdiff-cli report generate` returns STALE per-fn %** — it does not re-run the
  normalize / content-address step, so it can report a function unchanged when it actually
  moved (76%→85% shown as 76%). **Only `gruntz build` gives accurate per-function
  fuzzy%.** Never trust a bare `report generate` number.
- **A normalization refresh can expose fake `_$E<n>` matches.** The suffix is a
  compiler emission ordinal, and retail helpers may lack the relocation records
  needed to hash them like the base object. A broad loss of exact tiny
  static-init helpers after refresh is evidence that old ordinal pins or stale
  normalized copies inflated the report; keep the real static objects, remove
  the pins, and preserve their observed rows in
  `config/retail/compiler-generated-functions.tsv`. See
  `docs/patterns/volatile-compiler-ordinal-refresh-dip.md`.
- **Inline `.text` jump tables defeat objdiff alignment.** A function with one or two inline
  jump tables can't be aligned across the table region, so its *current* % measures
  alignment luck, not byte-correctness — **a byte-BETTER reconstruction can show a LOWER
  current %** (keep it; MAX-fuzzy preserves the best; see structure-over-current-%).
- **A function with NO `fuzzy_match_percent` key in `report.json` is at exactly 0.0%, NOT
  unpaired.** objdiff serializes with serde's skip-the-default rule, so a true 0.0 vanishes
  from the JSON and looks, by key presence, like a function it never diffed. It is not:
  objdiff's own `objdiff-cli diff … --format json` carries `"match_percent": 0.0` **with a
  live `target_symbol` link** for every one of them. **Always read the field as
  `float(fn.get("fuzzy_match_percent") or 0.0)`** — or, in-package, via
  `gruntz.verify.scores.fn_fuzzy()`. Two readers guessed and both guessed wrong, silently:
  `permute_sweep` defaulted the missing key to **100.0** and so skipped exactly the
  0%-matching functions from its worklist, and `Report.fn_pct` returned `None` so
  `gruntz sema rva` printed *no match line at all* for them. Pinned by
  `gate_selftest.TestOmittedZeroFuzzyPercent`.
- **The "delinker duplicate-symbol pairs the size-0 copy" story is DEAD — do not repeat it.**
  It is a stale diagnosis that outlived its fix and has since mis-explained the 0% functions
  to at least two lanes. The delinker really does emit ~2385 name collisions (a real `.text`
  def plus a size-0 UNDEFINED external reached through an ILT thunk), but `canonicalize_coff`
  has renamed the redundant undefined copy to `$dup$<name>` and retargeted its relocations to
  the real definition since `b1f3a0c52`/`1db37b32b`. Both sides now pair the real definition;
  in `grunt` 29 of the 30 symbols in the same packed `.text` section score normally.
- **The real cause of the residual 0% functions is a target/base symbol EXTENT mismatch.**
  Measured 2026-07-27, the whole population is **8 functions**, every one jump-table or
  thunk adjacent, and in each the delinked symbol's size disagrees with the base's because
  the carve is wrong — too long (`CFaderMgr::Add` 1926 vs 1408: it swallowed its neighbour),
  too short and ending mid-flow on a `jmp` instead of a `ret`
  (`CActionOptionsMenuBar::Refresh` 310 vs 290), or excluding the inline jump table the base
  symbol *includes* (`CGrunt::StepArrivalDrop` 2813 vs 2560 — objdiff renders the base tail
  as `.dword ?StepArrivalDrop@…+0x1f4` data rows). objdiff still scores these **0.0** even
  with 36–151 byte-identical instructions, so — exactly like the jump-table alignment bullet
  above — **their current % is not a proxy for byte-correctness.** This is a delinker/carve
  fix, not a source fix; MAX-fuzzy already records 0.0000 for them, so they cost nothing.

## Header axes: the transitive cone, and the `$S<n>` counter (2026-08-01, measured)

A **header** edit is not a `.cpp` edit with a bigger blast radius — it is a different kind of
change, and a search harness must treat it differently.

- **The cone is TRANSITIVE, not direct.** `include/Bute/ObjListBase.h` has exactly two direct
  includers, yet adding one empty inline `~CObjListBase() {}` moved five functions in units
  that include it only transitively (`Rez/RezList.h` → `Rez/RezMgr.h` → `Net/NetCmdSlot.cpp`).
  **Not one moved unit includes the header directly.** Measured A/B (dtor OFF → ON), reproduced
  independently:

      100.000 -> 89.535  netcmdslot|?Verify@CNetSession@@QAEHH@Z      <- the whole cost
       99.825 -> 99.614  sbi_rectonly|?LoadMainStatusBarSprite@...
       90.000 -> 89.975  play|?SaveUnderAndDrawCursor@...
       65.395 -> 65.429  imagerle16encode|?EncodeRle16@...   (+)
       56.000 -> 56.008  gamechecksum|?Checksum@CNetSession@@QAEHXZ    (+)
      exact 3215 -> 3214

  `netcmdslot`'s BASE obj md5 changed while its delinked TARGET obj stayed byte-identical, and
  `CNetSession::Verify`'s single reloc is the same on both sides — so this is purely the /O2
  declaration butterfly, not a reloc or pairing effect.

- **A header edit shifts MSVC's per-compiland symbol counter**, and file-scope statics embed it
  in their mangled name (`_kScrollRate$S41595` → `$S41607`, `_s_join$S29067` → `$S29079` —
  exactly +12 in every affected TU). Those names flow base obj → the Model → synth PDB
  → delink, so they stay self-consistent for ANNOTATED statics; an unannotated `$S` symbol has
  no such guarantee. Same mechanism as the `<new>`/`<new.h>` swap
  (`_s_FreezeRadius$S33024` → `$S32890`) — a general property of header edits, not one header's quirk.

- **Therefore, for a header axis the objective is TREE-WIDE, not per-symbol** — plus the reloc
  audit. Here the target became byte-exact (`~CRezList` = retail's 7-byte
  `mov [ecx],??_7CObjListBase@@6B@ / ret`) while the tree lost one exact elsewhere. A searcher
  optimizing only the target symbol takes that for the wrong reason; one optimizing only
  tree-exact rejects a change that fixed **4 link-breaking reloc defects**. `%` is structurally
  blind to relocs, so `assert_relocs` has to be part of the objective.

- **The DATA side of that blindness is worse, and now has its own gate.** A relocated word's
  bytes are a placeholder the linker overwrites, so both sides hold the SAME placeholder and a
  byte comparison cannot see a wrong referent at all — a vtable slot bound to the wrong method
  moves no byte. `gruntz verify data-relocs` (normal tier) adjudicates every pinned datum
  against the retail image's own `.reloc` table. Two traps it cost to learn: compare resolved
  ADDRESSES, never names (a name comparison must drop one-sided names to survive the pooled-
  literal split, and a wrong vtable slot is exactly a one-sided name — the first draft reported
  0 rows over 9806 words while being blind); and run BOTH sides through `resolve_thunk`,
  because retail's incremental link puts the ILT `jmp` thunk's address in a vtable slot, not
  the body's (4725 phantom rows without it). Details: `docs/data-attribution.md` §4.

- **Proposed cheap pre-oracle (plausible, not yet verified):** diff the Model. If a
  candidate shifts any `$S<n>` it disturbed the cone and needs the whole-tree rescore; if the
  CSV is byte-identical, a single-unit score should be sound. Sub-second gate in front of a ~40 s
  one — worth validating before relying on it.

- **Never write into `build/objdiff/base/` outside ninja.** An ad-hoc scoring harness that does
  leaves ninja unable to see the obj is stale, desynchronizing `report.json` from the source
  tree. Compile to a temp path and restore the source, the way `batch_source_variants` already
  does. This corrupted one lane's measurements and produced a mis-attributed ripple report.

- **REFUTED — the build DOES converge in one incremental pass.** A lane reported that after a
  header change the first build differs from the second on identical source (7 functions moving)
  and concluded a searcher must build to a fixed point before scoring. **Not reproducible.**
  Measured on a clean tree: three consecutive builds on unchanged source moved **0** functions,
  and two consecutive builds *after* a real header edit (removing the `~CObjListBase` dtor) also
  moved **0**. The non-convergence was a downstream symptom of the `build/objdiff/base/`
  corruption above, in the same lane's own harness. **Do not double-build before scoring** — one
  `gruntz build` is authoritative.

## Build / worktree state (pool worktrees carry stale build state across resets)

- **`build/` stale after a unit was REMOVED from `units.toml`** → ninja never GCs orphaned
  outputs, and the failure wears **three different masks**, so recognise the cause not the
  message: `vostok-delinker` failing `delink_data_section_manifest.tsv: storage does not
  match candidate section` (on `reghelpers.c`); the manifest re-enrolling a dead unit's
  vtables from stale COFF; and normalize dying with a plain
  `FileNotFoundError: build/objdiff/target/<stem>.c.obj` for a unit that is still live and
  whose delinked obj **is** present in `build/delink/named/` (seen 2026-08-09 on
  `debugprintf` after four units were merged away — 341 target objs against 369 delinked).
  Fix (build/ is gitignored): delete
  `build/gen/labels/<stem>.{csv,functions.json,globals.json}` + `build/objdiff/{base,target,normalized}/<stem>.obj`
  for every `<stem>` NOT in `config/units.toml`, then `rm -f build/objdiff/.delink.stamp`, rebuild.
  **`rm -f build/objdiff/.delink.stamp` alone fixes the third mask** and is the cheap first try.
  Unit merges are now routine (the TU-partition worklist has ~20 more), so expect this.
- **Stale `structs.json` once corrupted DATA extents — fixed.** DATA labels now take
  each declaration's size directly from pylibclang in the current TU; the delinker
  manifest no longer reads the whole-tree layout cache. `structs.json` remains an
  input to full-tier layout audits and the optional Ghidra viewer, and a full build
  refreshes it before those consumers run.
- **`build.ninja` stale after the include graph changed** — FIXED 2026-08-08; the note below
  is why you may still see it in an old tree. The `cl` edges DO carry per-header implicit deps
  (159 headers on `gamemode.obj`) and touching a listed header DOES rebuild — but the lists are
  baked by `gruntz configure`'s `local_headers()`, and the `configure` edge used to fire only on
  `config/units.toml`/`gruntz configure`. So a deleted/renamed header gave
  `ninja: error: <hdr> missing and no known rule to make it`, and — the silent, dangerous half —
  a NEWLY ADDED `#include` was absent from the baked list, so later edits to that header
  rebuilt nothing and you diffed stale code. **That is the failure any "byte-neutral header
  edit" claim rests on not happening.** Now every file the include scan reads is an implicit
  input of the `build.ninja` edge, and `gruntz build` runs `gruntz configure` unconditionally
  (~0.3 s, after memoizing a scan that used to cost 13.4 s) so a rename cannot wedge ninja.
  `/showIncludes` would give real depfiles but MSVC 5.0 rejects it (`D4002`).
  Manual escape hatch if a tree is still wedged: `rm -f build.ninja .ninja_deps .ninja_log
  .ninja_lock` (ROOT ninja state ONLY, NOT `build/` — keeps the expensive delink/Ghidra caches).
- **`GRUNTZ_SKIP_INIT=1`** before `nix develop -c <cmd>` skips the slow shell-entry `gruntz init`
  warmup — use it for quick one-off commands in a warm worktree.
- **Wineserver leak** — subagent builds leak `wineserver` processes; `pkill -9 -f wineserver`
  stale ones (they slow later builds). Don't chain build+format+status (can exceed timeouts).
- **`clang-format not on PATH — skipping format`** on commit is HARMLESS (formatting is
  whitespace-only / matching-neutral; it runs inside `nix develop`).

## Permuter / walls (see `docs/permuter.md` + `docs/patterns/`)

- **`permute.py` (operand-order/reassoc/decl-split) cannot move regalloc.** MSVC5
  canonicalizes `ptr+i == i+ptr`, so operand swaps are no-ops on SIB walls.
- **`gruntz permute state` targets cross-function compiler state, not arbitrary
  source bugs.** It perturbs the *preceding* TU content, so use it when a source-identical
  later function moves after TU composition changes. Do not assume independent COMDATs mean
  independent codegen: adding the real preceding `BlitIntoDesc` changed two `ShadeRect`
  loop schedules, including mask/shift order and `ax` vs `di` partial-register selection.
  Conversely, four previously tested intrinsic wall families did not move even at 1024
  variants. Use a controlled predecessor A/B test to classify the residue before spending
  a large state search; see
  `docs/patterns/preceding-function-state-recolors-later-comdat.md`.
- **A 95%+ "regalloc wall" is often a MISLABELED CORRECTNESS BUG** the diff masks. Audit
  before believing the `@early-stop`. This is the real yield lever (see the playbook below).

## The mislabeled-bug audit playbook (the actual % yield on near-100% functions)

Each recurred and banked exact/near-exact matches. Grep-able signatures:

- **Signedness** — retail `jb/jbe/ja/jae` where you emit `jl/jle/jg/jge` (usually a loop
  guard/unsigned compare → cast the guard to `u32`). **FP variant:** retail zero-extends an
  `i32` time/delta field before `fild` = `static_cast<double>(static_cast<u32>(x))` (`fild
  qword` vs your `fild dword`).
- **Wrong magic constant** — reciprocal divisor/multiplier hidden by objdiff's `<addr>` mask;
  verify `--base`.
- **Block-ordering / inline-vs-out-of-line path** (~+70pp, recurs a lot) — retail places one
  switch arm's body out-of-line where your source inlines it (or vice-versa); reorder to match.
- **`do-while` vs `while`** read loop; **`memcpy` vs `memmove`**.
- **Shared `goto fail` epilogue** — retail merges all gate-failures to ONE `return 0`.
- **Inlined static helper** — mark a static helper `inline` to reproduce retail's inlined loop
  (and it lifts every caller in that TU).
- **Missed CSE** — cl reloads `this->m_x` before each store (aliasing-conservative); hoist it
  into a local, matching retail's cached register.
- **Branch polarity** — retail makes the failure/shorter arm the fall-through.
- **`__cdecl` alias vs real `__thiscall` method** — a call modeled as a free `__cdecl` alias
  (wrong `this`, spurious CString ctor) that's really `obj->Method()` NRV `__thiscall`. Delete
  the alias extern, call the real method.
- **Duplicate compare** — a type-discriminator chain listing the same target twice → cl
  CSE-folds a `cmp` retail keeps distinct.
- **List-walk order** — retail's inlined `GetNext` advances first (`cur=node; node=node->m_next;
  use cur`) vs your process-then-advance.
- **COMPENSATING ERRORS — two bugs that cancel, leaving a "regalloc coin-flip".** A swapped
  argument pair colours the parameters into the opposite registers, so the pushes cancel and
  only the argument LOADS differ (`CButeMgr::GetString`). Wrong member offsets that fit in
  `disp8` where the right ones need `disp32` shrink the body by tens of bytes while `--diff`
  shows two operand mismatches (`CPlay::DrawDebugStats`, 812 bytes vs an annotated 862).
  Two cheap checks before you write an `@early-stop` on a register rename:
  `gruntz walls diagnose <rva>` (its first two lines are the compiled LENGTH of both
  sides - the invariant every instruction-aligned view is blind to), and a source diff against the
  function's SIBLINGS (a constant / argument order / offset that differs in exactly one
  member of an obvious family is a bug). See docs/patterns/compensating-error-signatures.md.

## Reconstruction targeting

- **Aim by UNIT-%, not function-%.** Mid-% functions in an ALREADY-reconstructed unit (high
  unit-%, e.g. ~90-95%) are genuine register/frame walls with no yield — a prior wave already
  fixed their shape. The shape-bug vein lives in LOW-unit-% units (`userlogic` 0.2%,
  triggermgrgrid 28%, gruntspawnconfig 41%, …) and `@stub` bodies (`rg '@stub' src`; each stub
  = net +1 exact).

## Codegen / modeling

- **`new X` inlines the ctor; placement `new (raw) X` does NOT** (MSVC5 /O2). To reproduce
  retail's inlined vptr-stamp + member init, use `new X`, not placement-new on raw storage.
- **Header butterfly** — adding a member (or an include) to a HOT header perturbs /O2 regalloc
  in that header's consumers; a "byte-neutral" fold across a 40-consumer header is NOT
  byte-neutral (it can net +/- a few exact via the ripple — measure). Enum-vs-int is neutral
  ONLY if the TU already includes the enum header (adding the include perturbs regalloc).
- **Retail was linked without ICF** (measured: 574 byte-identical functions at distinct
  addresses; `docs/linker-flags.md`). NOT because the linker lacks it -- LINK 5.10.7303 does
  advertise `/OPT:{ICF|NOICF|NOREF|REF}` -- but because retail did not fold. So two source
  functions with identical bodies are two functions. If a
  retail function shows up under two names at the SAME RVA, that's the fake-view symptom (the
  same body reconstructed on a fake view and on the real class); recover the one real identity.

## Cleanliness tooling (see `docs/cleanliness-metrics.md`, `docs/cast-metric-policy.md`)

- **`gruntz verify board`** computes the fast, comment/string-stripped
  `cleanliness-text-baseline.tsv` in the fast tier and the build/IR-derived
  `cleanliness-semantic-baseline.tsv` only under `--semantic` (the full tier). It prints
  measured rows with a delta; `--update` blesses text floors and `--semantic --update`
  includes semantic floors. The gate itself never writes a floor.
- Most metrics are at **0** (casts, placeholder classes/vtables/views, `)this`/`)m_`/`(char*)`
  casts, offset-cast macros — all DONE). Remaining actionable: **m_&lt;hex&gt; fields (~8.5k),
  Method/Stub/FUN/Gap (~245 unreconstructed stubs), `void* m_` members (18),
  `reinterpret_cast<class*>(m_)` (1), cpp extern decls (490)**.
- The one-shot cast CONVERTERS that drove those to 0 are gone; the ledger that keeps
  them at 0 is `gruntz verify casts`.
- Live gates run every `gruntz build` (fail the build): the `fast` tier (`board`,
  `vtable-bans`, `casts`, `compiler-artifacts`, `enum-domains`, `label-style`, `include-order`) and the
  `normal` tier (`unique-names`, `library-overlap`, `tu-order`, `data-tu-order`,
  `dead-code`, `undefined-closure`, `review-claims`, `data-relocs`, `data-access`,
  `data-coverage`), after the MAX gate. The vtable tier moved to
  `gruntz verify check --tier full`, and `gruntz verify selftest` is the
  negative-control harness. **Caveat:** a cleanliness
  regex can silently rot vs actual naming — a green `0` is a claim to re-verify against a fresh
  identifier enumeration, not proof.

## `objdiff-cli` is BUILT FROM SOURCE; the objdiff GUI is not (2026-08-09)

`flake.nix` builds `objdiff-cli` from `objdiff-src` (v3.7.3) with
`nix/patches/objdiff-bss-inferred-extent.patch`; the `objdiff` **GUI** is still the
upstream prebuilt download. Only the CLI generates `report.json`, so every number in
`gruntz verify status` / `README.md` comes from the patched build — but if you open the GUI on
a `.bss` section it will still show the old 50%-per-inferred-extent rows. That is the
GUI, not a regression. Same stale-shell rule as below: a shell entered before the flake
change still has the unpatched CLI on PATH.

## A new delinker patch needs a REFRESHED nix shell (2026-08-08, again 08-09)

Every `nix/patches/vostok-*.patch` changes `vostok-delinker`, so a `nix develop`
session entered before that commit still has the OLD binary on PATH and every
`gruntz build` dies in the delink step. Two sightings so far:

```
Error: candidate data COMDAT section 1 has no external offset-zero leader
        <- vostok-comdat-leader-nonzero-offset.patch
Error: <manifest>:2: storage does not match candidate section name/characteristics
        <- vostok-grouped-section-names.patch (`.rdata$r` is COFF's grouped-section
           form; the unpatched delinker demands a literal `.rdata`)
```

The third one (`vostok-legacy-data-not-into-comdat.patch`, 2026-08-09) fails
**SILENTLY**, which is worse: an old binary still appends every unplaced data
definition to the object's first COMDAT, so the delink succeeds and only the DATA
numbers are wrong (`fadereffects .rdata` 98.61 instead of 100.00). If a data
section you did not touch is a hair under 100, check the shell before the model.

That is not a source defect and not a manifest defect - it is a stale shell.
Re-enter `nix develop`, or run `nix develop --command gruntz build`. The same
applies in every worktree: they share the store, but each shell pins whatever
delinker path it resolved at entry.

**Confirm it in one line before you debug anything else** - if these differ, the
shell is stale and nothing else you are looking at is the cause:

```sh
readlink -f "$(command -v vostok-delinker)"   # what THIS shell resolved
nix eval --raw .#vostok-delinker              # what the flake says today
```

The trap has a nasty second half: it hides behind **`ninja 0.0s`**. A build whose
graph has nothing to do never reaches the delink step, so it reports green and
banks a scoreboard - and an integration "verified" that way has verified nothing.
When a build must prove an integration, check that ninja actually did work.

## Runtime triage: wine SILENTLY CONTINUES most of our faults (2026-08-09)

The first runtime evidence the project has. Reproduce with the user's own runner —
`WINEDEBUG=+seh,+loaddll ~/gruntz-wine/run.sh` plus `autokey.sh` (attract mode waits for
Enter) — and read the log with two rules in mind.

**One visible crash hides hundreds of invisible ones.** wine's `krnl386.exe16` installs a
vectored exception handler; on our access violations it returns `ffffffff`
(`EXCEPTION_CONTINUE_EXECUTION`), so the faulting instruction is stepped over and the game
runs on with garbage in the destination register. A single 260 s run dispatches ~1400
`c0000005`. Count them, do not stop at the one that reached the debugger:

```
grep "dispatch_exception code=c0000005 .*addr=[0-9A-F]\{8\}$" log \
  | grep -o "addr=[0-9A-F]*" | sort | uniq -c | sort -rn
```

**Always run the control.** `run.sh retail` — same prefix, same assets, same keystrokes —
logs **0** access violations. That is what licenses calling any of ours a real defect.

Faults observed in the candidate, in first-occurrence order (addresses are candidate VAs;
map them with `build/exe/GRUNTZ.candidate.map`):

| site | access | note |
|---|---|---|
| `LayerBlitFrame+0x6a,+0x7f,+0x86,+0x8b,+0x90` | read `[src+0x2c/0x1c/0x18/0x14/0x10]` | `src` (a `CImage*`) = 0x01e2b3a0, unmapped. 3 calls x 5 field reads per burst == `CPlay::BuildHelpReveal` with `m_revealFrame == 1` |
| `CDDSurface::BltFast+0x12` | read `[src+8]` | downstream: LayerBlitFrame handed it the emulated garbage |
| `CPlay::LoadByMode+0x84e,+0x851` | read `[0]+0x2c`, `[0]+0x28` | `CGameLevel::m_mainPlane` is NULL — see below |

`LayerBlitFrame` 0x115300, `CDDSurface::BltFast` 0x13ef90, `CPlay::BuildHelpReveal`
0xd72c0 and `CPlay::LoadLoadingBarSprite` 0xd7440 are all byte-identical to retail, and
retail's `CPlay` ctor also leaves `m_revealCap*` uninitialized (verified: the ctor 0x8c9d0
stores to +0x4b0 and +0x4cc and to nothing between them, on BOTH sides) — so the bad
`CImage*` came out of the loading-bar worker, not out of the code that reads it.

**But the repeated value is the array's `m_pData`, not an element** — see
[`continue-on-fault-retains-the-base-register`](patterns/continue-on-fault-retains-the-base-register.md).
`LoadLoadingBarSprite` reads each frame as `mov edx,[ecx+0x14]` (the `CObArray` data
pointer) then `mov edx,[edx+N]`; when the second load faults it is stepped over, so `edx`
keeps `m_pData` and *both* `GetAt(1)` and `GetAt(2)` store it. That is why the two differ
by nothing. There is no fill that puts one pointer at two indices — `InsertFrame` 0x151f00
refuses a non-null slot and every frame path allocates a fresh `CImage`. So the question is
"what makes `CObArray::m_pData` bad", not "which writer stored a bad `CImage*`".

**The `CDDrawWorker`/`CImage`/`CObArray` object graph is NOT the defect — do not re-audit
it** (swept 2026-08-09, all evidence static):

| checked | how | result |
|---|---|---|
| object sizes | `gruntz verify alloc-size` — retail `push <n>; call ??2` vs Clang-computed `sizeof` | 430 attributed sites. `CImage` 0x34 (0x151f24), `CDDrawWorker` 0x6c (0x154b24), `CDDrawWorkerHost` 0x158 (0x15d8ef), `CPlay` 0x520 |
| member offsets | a subobject/`this`-offset census (831 agree / 0 disagree, 0 past-sizeof; instrument retired, `gruntz verify layout` is the live field-offset oracle) + hand-check vs retail | `CDDrawWorker`: `CObArray` @+0x10, `m_pData` @+0x14, `m_nSize` @+0x18, `m_name[0x40]` @+0x24, `m_minIndex` @+0x64, `m_maxIndex` @+0x68. The "+0x10 vs +0x14" discrepancy in the first write-up is not one: +0x10 is the sub-object (`lea ecx,[esi+0x10]`), +0x14 is `m_pData` |
| vtable slots | the hierarchy/owner audits 0 flags, 0 MISBOUND / 0 RTTI-MISBOUND, all 2887 slots wired (now `gruntz verify vtables` + `gruntz sema class`) | `CImage` 18 slots, `CDDrawWorker` 17 — both match RTTI and our headers in order. `InsertFrame`'s `[edx+0x2c]` is `CImage::Resolve`, `[edx+0x4]` the deleting dtor |
| ctor init | retail's inlined worker ctor in `InsertFrameByKey` 0x154ae0 | vptr, `m_id`, `m_flags=0`, `m_ownerCtx`, **`call CObArray::CObArray()` on `this+0x10`**, vptr, `m_minIndex=0x1869f`, `m_maxIndex=0` — exactly our header's inline ctor. `Unload` 0x151ee2 resets the same pair |
| writers | whole `ddrawworkerregistry` unit is 100.00; `AddFrameAt` 0x1521c0 has **no rel32 caller** (inlined everywhere); we define no MFC container method, so `m_pData` is only ever written by `nafxcw:array_o.obj` | every deleter (`RemoveByKey`, `MapTeardown`, `RemoveWithPrefix`) removes the map key *and* deletes — no dangling registry entry |

Vtable "slack" in `link_sections --undersized` is a false lead for this: retail pads between
`.rdata` contributions with zeros, so `??_7CPlay@@6B@` reads `claim 0xac -> next pin +0xd8`
while the real vtable is exactly the 43 slots we emit.

`gruntz verify link-tier` reports 0 PHANTOM / 0 UNDEFINED-DATA / 0 MULTIPLY-DEFINED and
**11 DIVERGENT COMDATs** — all characterised as toolchain-flag divergence, none semantic:
`??_7CImage@@6B@` (72 B in `wwdgameobject` vs 76 B in `cimagecomdats`) is the same 18 slots
in the same order, differing only by the `??_R4CImage@@6B@` COL prefix `/GR` adds, and the
symbol correctly points past it (+4) in the `/GR` copy; `??1?$CArray@PAU…` 48 vs 96 B is the
same body plus a `/GX` frame; `??_GzPTree` 32 vs 80 B is call-the-out-of-line-dtor vs
inline-it. Real byte-fidelity debt, but the linker cannot pick a wrong *behaviour* here.

So the loading-bar array is a **victim, not the cause**. With ~1400 stepped-over faults the
process corrupts itself progressively, and every register left stale by a skipped load feeds
the next store. Triage the FIRST fault in the log — and prefer the first `info[0]=1` (write)
fault, since that is what can damage an unrelated heap block.

**The third row is the sharper lead, and it is a different subsystem.** The pair of reads is
retail's `mov ecx,[eax+0x2c]` / `mov edx,[eax+0x28]` at 0xcaaa4/0xcaaa7, where
`eax = m_world->m_level->m_mainPlane`; note the two sites just above it (0xcaa5e, 0xcaa70)
DO guard `m_mainPlane != NULL` and this one does not, so retail expects it set by then.
`CGameLevel::m_mainPlane` is `+0x5c` — confirmed against `ReadPlane` 0x15d8d0, which is the
only writer: `test BYTE PTR [edi+0x8],0x1` (the plane's `CLoadable::m_flags`) then
`mov [esi+0x5c],edi` / `mov [esi+0x60],ecx`. Our `ReadPlane`, `ReadObjectPlane` and the
`CDDrawWorkerHost` ctor are all 100.00, and `CDDrawWorkerHost::Read` 0x161640 (96.72) copies
the header flags with the identical `mov eax,[edx+0x8]; mov [ebp+0x8],eax` — its residue is
scheduling plus a `repne`/`repnz` mnemonic alias. So NULL means no plane carried bit 0, i.e.
the WWD plane records reaching `Read` are wrong: chase the level decode
(`_WwdFile_InflateMainBlock` 0x160790 is 88.74, `CGameLevel::LoadWwd` 82.41), not the plane
objects.
