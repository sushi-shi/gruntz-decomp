# Focused helper source follow-up

This bounded pass examined source helpers alongside PR #79. Its purpose is to
apply evidence-backed source layers, not to count an audit as a matching win.
Per-candidate adoption/rejection evidence belongs in
`config/lithtech_lineage.tsv`; this document references the stable IDs.

## Applied-candidate queue

| Candidate IDs | Gruntz targets | Source layer |
| --- | --- | --- |
| `hs-rng-one-bound` | `DispatchDemoMoverLogic`,0x3c300 | Missing signed one-bound random overload and two actual consumers |
| `hs-rng-shuffle` | `CPlay::ScanShuffleQuads`,0xd9290 | Existing two-bound helper at three expanded selection sites |
| `chance-ghost-range-preserve`, `chance-brick-range-preserve`, `brick-rng-three-stack-range` | Ghost search and two/three-brick layer selection | Nine actual uses of the existing inclusive-range helper |
| `hs-bitset-body` | zBitVec string constructor,0x16d3a0 | Surviving zBitSet small-buffer pointer accessor |
| `hs-bitset-error-tail` | Same constructor | Existing error-helper boundary at three expanded error tails |
| `bute-vector-range-reference-api` | Six Bute getters/setters, two item constructors, parser | Reference API and ordinary bound temporaries |
| `hs-error-owner-identity`, `reassess-zerrhandling-all-error-sites` | Error, tree, array and bitset family | Original typed error owners, const helpers and static storage scope |
| `reassess-zvec-*`, `reassess-zdvec-*`, `reassess-zdarray-*` | Shared array base and typed declaration family | Protected API, source widths/names, bounds and extension helpers |

The one-bound candidate reopens the broad omitted-overload conclusion in
`blood2-getrandom-range`: actual one-bound consumers exist. The implementation
must preserve CRT `rand()`, signed remainder and the distinct exceptional cases
of the one-bound and two-bound overloads. `GetRandomNumber()` is another generator.

The independent source copies are Shogo's `RiotCommonUtilities.cpp`, Blood II's
`ClientUtilities.cpp`, and NOLF/NOLF2/F.E.A.R.'s `CommonUtilities.cpp` at revision
`845119c`. AVP2's v1.0.9.6 `CommonUtilities.h` additionally supplies inline
visibility evidence. No reference source files are redistributed.

Real-TU results: the RNG include-only,
named-bound, direct-bound and helper-use controls remained flat at 97.207146 for
Demo and 99.84772 for Play, with unchanged topology. The authentic helpers are
kept as source restoration, not advertised as matching wins.

The fresh constant-range checkpoint `8c9d03e92` preserves the ghost search's
complete normalized body and ordered references. The brick caller recovers its
current 90.2473% historical bank; composing all four three-stack samples is then
byte-flat. Full build/MAX gates pass with no fresh failures, all historical maxima
remain, and no new exact function is claimed. Controlled source forms are in the
[range-helper pattern](patterns/rand-modulo-peel.md); canonical candidate evidence
is in the three adoption rows above and `brick-rng-shogo-fifty-oracle`.

The reconsidered alternatives remain individually recoverable under `chance-x-*`,
`chance-deferred-*`, `reassess-chance-*`, and `brick-rng-*`. The defender/toy range
consumers and variable-bound color helper remain pending. This does not close
`reassess-israndomchance` or any of the 16 broad family reassessments.

For the bitset constructor, the accessor-only A/B remained 95.66782, and composing
the existing error helper at all three error tails also remained 95.66782. Moving
the real `maxv=0` initialization below the null/default-size guards then matched
the normalized retail constructor and removed its EH row from the nonexact list:
0x344 bytes, 289 instructions, 24 calls, 48 branches, 1 return, 34 relocations. The retail
late zero reuses the dead incoming-token home. The full build and MAX check pass after banking; the primary source fingerprint
75e65af7cd6e has MAX 100. The one-function raw referent audit finds no defects,
and the single unwind action matches. Source checkpoint: `9f75d37ce`. No class rename or ABI change was part of that composition.

The bitset queue comes from NOLF's `ztools.h`, not the earlier exact-name
paired-library census. Its proposed full-family work is separately tracked by
`hs-bitset-complete-identity` and `hs-error-owner-identity`; do not silently mix
class renaming, unsigned/ref ABI changes or extra methods into a helper test.

## Recheck index

All screened-out decisions are revisitable. Their reasons, evidence, tests and
reopening criteria live in the following canonical candidate rows. A provisional
lack of evidence is not disproof, and no candidate is rejected merely for a
lower score.

- Random boundaries: `hs-rng-zero-arg`, `hs-rng-float`.
- Bitset APIs: `hs-bitset-extra-members`, `hs-bitset-extra-empty`,
  `hs-bitset-extra-truth`, `hs-bitset-extra-dump`, `hs-bitset-extra-mode`,
  `hs-bitset-extra-capacity`, `hs-bitset-extra-compare`,
  `hs-bitset-extra-difference`, `hs-bitset-extra-intersection`,
  `hs-bitset-extra-disjunction`.
- Allocator owners: `hs-structbank`, `hs-objectbank`.
- Collection owners: `hs-clinkedlist`, `hs-cglinkedlist`,
  `hs-fastlinkedlist`, `hs-moarray`, `hs-bankedlist`, `hs-fear-fastlist`.
- Factory owner: `hs-fear-factory`.
- Color boundaries: `hs-color-vector-word-forward`,
  `hs-color-vector-word-reverse`, `hs-fear-argb`.
- Stream owners: `hs-fileio`, `hs-abstractio`.
- Path helpers: `hs-helpers-path-upper`, `hs-helpers-path-full`,
  `hs-helpers-path-path-file`, `hs-helpers-path-file-ext`,
  `hs-helpers-path-names`, `hs-helpers-path-next-dir`.
- Name collision: `hs-blood-timebomb`.

## Reopened reassessment — required, not completed

The earlier pass incorrectly treated existing ledger entries as sufficient to
exclude families from fresh review. Their old conclusions are evidence to
challenge, not settled decisions or proof that the helper search is complete.
The following families are explicitly reopened in the wave-16 lineage queue:

- Vector and rectangle initialization: `reassess-vector-init`, `reassess-rectangle-init`.
- Complete value types: `reassess-cavector`, `reassess-carange`, `reassess-crange`.
- Math/helper boundaries: `reassess-min`, `reassess-max`, `reassess-clamp`,
  `reassess-interpolate`, `reassess-israndomchance`, `reassess-sqr`, `reassess-round`.
- Complete container families: `reassess-zdarray`, `reassess-zsymtab`,
  `reassess-zptrcoll`, `reassess-zptree`.

Reassessment must revisit complete declarations and use families, excluded
methods, constructor/initialization sites, widths and ownership, nested helper
boundaries, and current retail evidence. Prior adoption, an old rejection, or an
exact function does not by itself establish that the complete source family is
correct or fully modeled. Preserve old tests and reasoning, but record a fresh
disposition with evidence for each candidate; do not reject a composed source
layer merely because an isolated earlier trial lowered a score.

The fresh source review has identified concrete execution targets without
completing those four broad value/initialization reviews:

- Complete Bute vector/range API: `bute-vector-range-reference-api`,
  `nolf-bute-private-getters`, `nolf-bute-boundary-revisions`.
- Vector initialization sites: `reassess-vector-activate-sites`,
  `reassess-vector-bound-pairs`.
- Complete constructor/operator/copy compositions: `sdk-vector-constructors`,
  `sdk-vector-spotlight-minus`, `sdk-vector-spotlight-plus-equals`,
  `sdk-vector-assignment`, `reassess-older-vector-copy`,
  `reassess-older-vector-byvalue`.
- Rectangle construction and consumers: `sdk-ltrect-constructors`,
  `reassess-rectangle-query-cell`, `stdlith-morect-grid-setup`.

The Bute reference API is now applied in `9e2264c1a`: all nine affected bodies
remain exact, with unchanged ordered referents and 31 unwind actions. This is
source fidelity, not nine new matching wins. Its canonical decision overturns
the old pointer-only rejection. The pattern
`generated-symbols-do-not-prove-reference-api.md` records the verification method.

The wave-16 entity rows separately retain scoped no-evidence findings and
deferred vector/rectangle APIs. The earlier rejection rows retain their actual
tests with reopening qualifications. Other pending families still require
complete use-site tests.

The math reverse-use pass adds concrete pending consumers under
`reassess-sqr-*`, `reassess-ambient-clamp`, `reassess-ambient-min`,
`reassess-scroll-clamp`, `reassess-motion-max`, `reassess-motion-min`,
`reassess-flash-interpolate`, `reassess-flash-min`, `reassess-chance-*`,
`reassess-crange-*`, and `reassess-round-*`. Scope-limited rejected mappings
retain reopening criteria in those same canonical rows; they do not close
their broad helper families. No new compiler result is claimed for that pass.

The container review adds individual pending rows under `reassess-zminerr-*`,
`reassess-zerrhandler-*`, `reassess-dhandler-typed-layout`,
`reassess-zerrhandling-*`, `reassess-zvec-*`, `reassess-zdvec-*`,
`reassess-zdarray-*`, `reassess-zptrcoll-*`, `reassess-zptree-*`,
`reassess-zsymtab-dtf-placement`, and `reassess-zsilent-consumers`.
The original-library controls and reader provenance are retained in
`nolf-zptree-error-owner`. The error-owner and forwarding restoration is now
applied in `a57322345`, with fresh decisions in the individual candidate rows.
All 45 exact primary/EH rows in TypeKeyColl remain exact; this is source fidelity,
not 45 new matches. The full build passes and the raw referent audit finds zero
defects across 3,915 near-exact functions. The verifier regression suite ran
463 tests successfully, with two skipped.

The complete container reviews remain open, as do all 16 broad reassessment
families. In particular, `reassess-zerrhandler-set-debug-locals` is an applied
source layer, not an exact-match or exhausted-search claim. The two genuine
retail startup functions tracked by `reassess-dhandler-startup-owner` remain
unmatched; their unresolved source ownership must not be concealed by a
classification or invented constructor. Further source work starts with the
complete array declarations and typed consumers, not a blanket exemption for
helpers that were previously considered.

The complete array declaration/API checkpoint is applied in `3c4eec033`.
Its eight narrow adoption decisions are in the corresponding wave-16 rows;
`reassess-zdarray` itself remains pending. Fourteen selected real-TU controls
retain their scores, including both exact typed indexers, the exact raw accessor
and the exact lifetime family. Growth remains 91.8584%, and the three large
consumers still retain 45/52, 2/11 and 4/10 required typed calls. No matching gain
is claimed for this source restoration.

All 22 real-VC5 header tests pass, including negative access checks and the
extension's two construction bands. The full verifier ran 464 tests successfully
with two skips; the full build passes, and the raw near-exact audit still reports
zero defects across 3,915 functions. Banking retains all 4,429 RVA-keyed historical
proofs and all unchanged-source MAX values. Rejected return-API inferences remain
recheckable under `reassess-zdvec-normalized-success` and
`reassess-zdvec-pointer-api-inference`.

## Coverage and remaining work

The search combined header inline/function-like-macro/template/in-class-method
screens with reverse-use searches for random arithmetic, small-buffer bitset
selection, error tails, container ownership, file APIs and delete/nulling sites.
Definitions and Gruntz owners were read for the concrete candidates, and three
selected targets received pinned read-only call/CFG/referent diagnostics.

The source-file discovery sets contained 493 Shogo, 600 Blood II, 744 NOLF, 1426 NOLF2,
1893 F.E.A.R., 1731 AVP2-release and 903 NOLF-release `.h`/`.cpp` paths. These counts
are inventory coverage, not a claim that every body was reviewed. Macro-generated
entities, uppercase-extension files, unexamined class families and other released
archives remain open. No whole-tree AST equivalence or binary-wide absence claim
was made. No VC6 or F.E.A.R. instruction scheduling is imported into VC5 reasoning.

Two additional RNG use families remain to select and diagnose per owner:
GruntSteps' endpoint-fallback selection, and LoadExplosionSprites' constant-bound
call. The zero-range-plus-increment family in BattlezMapConfig/TriggerMgr also
requires its own semantics/byte-width audit; it must not be mechanically replaced
by either known overload. These are open observations, not accepted/rejected
source transfers.

The broader constructor/lifetime pass can investigate the source-proven guarded
delete/nulling pattern. AVP2-only macro evidence that cannot honestly be represented
by a Git revision is recorded as tagged archive/file SHA256 provenance in the
canonical ledger: `hs-term-delete`, `hs-termsurf`, `hs-termstring`, `hs-termobj`.
Extracted-file verification checks the header payload; it does not re-verify
the archive digest. No Git commit identity is fabricated.

Before declaring a candidate accepted: compile the real owner TU, inspect the
first real divergence and raw ordered referents, preserve semantics and helpers,
then run the full pinned build and MAX gates. A helper restoration may be
matching-flat; report that honestly rather than call it a score improvement.

The next caller composition restores shared action-phase helpers in
`Gruntz/GruntActionInline.h` for FinishActiveAction and StepArrivalCommit.
Commit `5dd7fe288` records the implementation; the four `grunt-two-callers-*`
ledger rows identify only these fresh, tested adoptions, not whole-family closure.
Real-TU controls move them from 57.1871/66.9022 to 69.6296/83.0360, respectively,
without altering the complete ZTools array implementation. The controlled
[phase composition](patterns/animation-name-accessors-restore-template-call-cuts.md#complete-action-phases-after-the-sourced-array-api)
records intermediate dips, source forms and remaining call-set gaps.
The `reassess-grunt-glp-*` and `reassess-grunt-tube-*` ledger queues retain the
unreviewed sibling applications and larger conditional operation. Narrow
incompatible replacements and their reopening criteria are recorded only under
`grunt-glp-*` and `grunt-tube-*`. These are Gruntz sibling-source hypotheses,
not claims that externally released source contained these helper names.
All sixteen broad helper families remain open.

The next composition adds complete pickup and knockback helpers plus named
conditional action macros. It restores Arrival's complete ordered lookup
topology at 91.1094%; Finish reaches 78.4463% with J/N/K cuts still open.
Implementation `2c5021e25` is recorded narrowly by
`grunt-two-callers-pickup-application`, `grunt-two-callers-knockback-settlement`,
`grunt-two-callers-conditional-action-macros` and
`grunt-arrival-bomb-termination-macro`.
Preserving the caller's M comparison result inside that complete macro raises
Arrival further to 91.4820 without changing its recovered lookup topology.
The [controlled follow-up](patterns/animation-name-accessors-restore-template-call-cuts.md#complete-pickup-knockback-and-bomb-phases-recover-arrivals-call-topology)
records the real-TU sequence. `arrival-m-*`, `arrival-o-*` and `arrival-j-*`
retain individual provenance, incompatible transfers and deferred alternatives;
the `grunt-*-conditional-control`/`grunt-*-nested-pickup-control` rows retain
the tested source-form exclusions and reopening criteria. This is narrower than
complete consumer-family adoption and does not close the broad reassessments.

Fresh entrance reassessment adds the `fk-*` queue, including independent
collision, postcommit, occupancy, identity, safety-timer and configuration
responsibilities. Its pending candidates and narrow exclusions retain source
provenance and reopening criteria in the canonical ledger. The actual tree now
shares collision and postcommit operations, preserves the complete interrupted
drop and conditional pickup as macros, and repairs the natural current-player
guard. The real-owner/original-executable guard test also exercises unchanged-
count negative controls.

Implementation `dc7394d28` is recorded only by the narrow adoption rows
`fk-reset-current-player`, `fk-resolve-entrance-occupant`,
`fk-postcommit-entrance`, `fk-interrupted-k-effect` and
`grunt-two-callers-complete-j`. They do not resolve the broader conditional K,
grid/identity/timer/configuration or other J-consumer queues.

This composition moves Finish from 78.4463 to 83.1934 with J recovered and N/K
still open; Arrival retains its complete lookup topology at 91.4676. Natural
entrance is 88.3289 after the corrected guard and complete helper composition,
with its retail-sized frame restored. These are partial source/matching changes,
not whole-family closure or an exact-match claim. The sixteen broad reassessment
rows remain pending, including every family named in the original correction.

The next scoped implementation, `684215cdc`, reuses the existing typed occupant
getter and adds complete release/acquire helpers to both entrance owners.
Canonical adoptions are `grid-entrance-occupant-use` and
`grid-entrance-occupancy-mutators`; the `grid-*` queue retains the wider owner,
consumer and source-form questions and every scoped exclusion. Finish now has
all eleven retail typed lookups and a new historical best of 91.0935, with no
extra game calls. The real-TU declaration-only negative control and remaining
residue are recorded in the
[occupancy composition](patterns/animation-name-accessors-restore-template-call-cuts.md#typed-occupancy-operations-restore-finishs-remaining-call-boundaries).
Full build/MAX gates and 30 focused tests pass. This still does not close any
of the sixteen broad reassessment families or make the PR merge-ready by itself.
