# Constructor and lifetime cleanup: first reviewable slice

This PR is stacked on the template/container work in #79. It starts the broader
cleanup with the bounded leaf/wrapper family previously investigated in
`6fc68fb0d`; it does not combine all subsequent constructor-family rewrites.

## Source changes

Seven handwritten empty derived destructors become implicit: `CAmbientPosSound`,
`CRandomAmbientSound`, `CMovingLogic`, `CUniformTileImageSet`, `CRectTileImageSet`,
`CWwdGridIter` and `CWwdGridShell`. Existing emitting-TU bindings remain in place.

Nine empty default constructor wrappers are unnecessary: the two ambient-sound
leaves, `CDemo`, both concrete Gruntz command wrappers, `CInputDevBase`,
`CUserBase`, `CWwdGridShell` and `StreamVoiceFeeder`. Their complete declarations
have no competing constructor overload that would suppress implicit default
construction. No initializer, method body or virtual slot is moved to a fake
helper. Source ambiguity between equivalent empty and implicit declarations is
not claimed to be recoverable from matching bytes alone.

Authored base boundaries remain. A fresh real `SerialObjectFactory.cpp` control
with its configured `/O2 /MT /GX /GR` flags confirms that omitting
`CUserLogic::~CUserLogic` removes its vptr store and shrinks 68 bytes to 62.
`CUserBase` and `CGruntzCommand` still declare the destructors introducing their
virtual slots. Trivial EH-target destructors were not removed in this slice.
The earlier claim that static initialization proves the `TypeKeyRec` constructor
is superseded by the complete `_dhandler` source review; current dispositions
are `reassess-dhandler-implicit-startup` and `reassess-dhandler-startup-owner`
in the lineage ledger.

The reusable lifetime rules and corrected historical claims live in
[empty special members](patterns/empty-special-member-calls-and-vptr-stores.md),
[vptr restamping](patterns/eh-dtor-vptr-restamp-presence.md), and
[the former manual init interpretation](patterns/vptr-stamp-void-init-not-ctor.md).

## Controls

Run after a full build in the pinned environment:

```sh
nix develop -c python3 -m unittest discover -s scripts -p test_implicit_lifetimes.py -v
nix develop -c python3 scripts/audit-template-models.py --workers 4
```

The lifetime tests compare all seven complete normalized retail bodies and
ordered referents, and compile both sides of the load-bearing base-destructor
negative control in a disposable include overlay. They do not modify repository
headers or execute the game. The regular container, census and compiler-artifact
controls remain applicable. Current-score changes in unrelated callers are not
used to select constructor spelling.

## Still separate

The shared game-logic registration/constructor family (`754a2501f`) and canonical
Wap/WWD constructor family (`0c5763709`) need their own current-context review and
integration. Their old branch-wide baselines are not imported. Broader SDK adapter
changes from the later local stack are also outside this first lifetime slice.

## Current-context validation (2026-09-21)

The full build passes MAX and all fast/normal gates. All 4,429 historical RVA
maxima and the same three absent rows are retained; no implementation best is
reset. Four fresh unchanged-source caller deltas were diagnosed as allocation/
scheduling with matching call/branch/return topology before banking. No caller
was respelled to recover an aggregate score. Current started-unit results are
3,788/4,426 exact and 93.66% fuzzy.

All 37 tests pass: two lifetime controls plus the 35 inherited controls from
#79. All seven destructors match complete normalized retail bodies and ordered
referents; five also pass the simple raw resolver, while the two EH-bearing
bodies require the canonical handler identities. The 293-object before/after
census loses or adds no emitted function identity.

The fresh audit covers 282 TUs, 4,766 definitions and 31,553 use sites with no
parse errors or uncovered files. Six changed review contexts were revalidated
against unchanged definitions and exact normalized pairs. Nineteen new decisions
record the sixteen omitted methods and three retained base destructors. The
5,694-row queue has no stale, orphan or invalid reviews; its 74 current decisions
do not certify the remainder of the queue complete.
