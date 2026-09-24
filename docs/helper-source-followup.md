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

Previously adopted/rejected math, typed containers, direct library owners and
game-class analogues remain their existing individual lineage rows. They were
not counted again as new candidates. In particular, the vector/rectangle Init,
CAVector/CARange, min/max/clamp, CRange, INTERPOLATE, IsRandomChance, SQR/ROUND,
zDArray, zSymTab, zPtrColl, zPTree families already have durable decisions.

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
