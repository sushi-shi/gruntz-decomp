# The EH unwind map is a C1 fingerprint: dead TRACEs allocate states and caller cb

tags: cpp:trace cpp:temp cpp:inline | asm:funcinfo | topic:codegen-idiom topic:wall
symptoms: `gruntz.audit.eh_band` reports a C1 state-count or map-topology
difference; a retail-only NULL-action state with no corresponding state store
is evidence for an eliminated destructible scope, while action and funclet
differences expose ctor/dtor inline cuts, object identity, and frame layout
confidence: 9/10

`_s_FuncInfo.maxState` counts C1's destructible-scope allocations, and C2
cannot subtract from it: a scope whose code is eliminated keeps its unwind-map
entry with a NULL action and no state stores. Release MFC defines `TRACE` as
`1 ? (void)0 : ::AfxTrace(...)`; its arguments are parsed, typed, and
STATE-ALLOCATED before C2 deletes the false arm. Two consequences are measured:

1. **A dead destructible temp leaves a map-only state.** A TRACE argument that
   builds a `CString` (a by-value `GetName()`, `CString(...) + "..."`)
   allocates one state per temp; two temps in one full expression nest
   (`26 -> 25`). The retail state can survive with a NULL action and no code.
2. **A dead TRACE still adds caller cb.** The inline budget is
   `clamp(2*cb(caller), 1000, 35000)` (see
   `inline-budget-emits-ool-comdat.md`), and cb is a front-end statement-mass
   estimate that counts the dead statement. In `CGruntzMgr::TransitionState`,
   dead TRACE mass re-enabled retail's `CCreditsState` ctor expansion; its
   base guard, `CRgn`, and `CString` states then materialized.

## Exhaustive current sub-100 assessment (2026-08-13)

Run:

```text
python -m gruntz.audit.eh_band --nonexact \
  --nonexact-tsv /tmp/eh-nonexact.tsv
```

The audit joins all 827 current sub-100 scoreboard functions to the complete
750-group EH band and compares state count, every `(toState, has_action)` map
row, canonical funclet code, ordered teardown targets, and frame displacements.

| classification | functions | conclusion |
|---|---:|---|
| no retail EH map | 600 | pattern inapplicable; this is not a clean verdict |
| EH fingerprint identical | 184 | no destructible-scope, nesting, teardown, or frame-layout defect visible to this oracle |
| frame layout differs only | 35 | C1 structure and teardown agree; repair the owning frame/local model |
| C1 state count differs | 5 | structural scope or inline-cut defect |
| C1 map topology differs | 1 | equal `maxState` hid different scope nesting |
| teardown target differs | 2 | object/destructor/delete identity or inline-cut defect |

All five count differences have **more candidate states than retail**, so none
has the retail-only map state required to infer a missing dead TRACE temp. They
are `CButeMgr::SetString`, `CChatBoxOwner::ProcessCheatInput`,
`CDDrawSurfaceMgr::RestoreChildren`, `CDDrawSurfaceMgr::SnapshotChildren`, and
`CPlay::LoadGameAssetNamespaces`; their bodies and call sets must decide which
scope or inline expansion is surplus. The sole equal-count topology difference
is `CGruntzMgr::ChangeState`: 7 of 11 map rows differ because its
`CMoviePlayer` ctor/dtor inline cuts nest differently. The target-only rows are
`CGruntzMgr::Run` (class delete versus `CObject` delete) and
`CFontConfig::MeasureLabel` (`CPen` versus `CGdiObject` teardown).

Therefore the destructible-temp half of this pattern proposes **no remaining
source addition among current sub-100 functions**. The caller-cb half is not
exhaustively decidable from EH: a dead statement with no destructible temp can
change inline budgeting without leaving a map state. Require independent
call-set/inline-budget evidence before adding one. An identical fingerprint
does not prove the whole function correct, and the 600 mapless functions yield
no evidence either way.

Primary code exactness is also not proof. `CKeyedList::AddNode`, outside this
sub-100 census, has exact primary code while its owner-TU C1 state count remains
2 versus retail's 3. The missing NULL state and exact bytes could not be
satisfied together by any authentic TRACE-temp spelling tested; retain the
structural discrepancy rather than calling the source proven by its score.

## Reading the fingerprint safely

Each `new T` with a throwing ctor contributes a delete-guard state
(`??3@YAXPAX@Z`). An inline-expanded ctor chains its during-ctor states under
that guard in member declaration order. A funclet body's `add ecx,<disp>` names
the member offset. `??_M@YGXPAXIHP6EX0@Z@Z` with a pushed dtor callback proves
an array member `T[n]`; its arguments expose dtor, count, `sizeof(T)`, and the
member address.

Compare the full map, not only `maxState`: `ChangeState` proves equal counts can
hide different topology. Compare the candidate COMDAT from the retail group's
owner TU: header-inline copies can have TU-specific C1 maps, and an arbitrary
duplicate falsely reported `CMoviePlayer::~CMoviePlayer` as 4 versus 5 even
though the owning `creditsstate` copies are 5 versus 5. Canonicalize both
`[ebp+disp8]` and `[ebp+disp32]` frame operands before comparing funclet code;
otherwise a large frame shift looks like a different teardown body.

Costing caveat: a TRACE temp or named conversion may add a lifetime-flag
zero-init and flip cl's zero-register heuristic. VC5 passes an lvalue `CString`
through varargs bitwise, producing no temp or state. Dead string literals do
not enter the final string pool. Statically unreachable code after `return`
allocates no state because C1 drops it before state allocation, so it cannot
substitute for TRACE.
