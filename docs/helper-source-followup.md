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
