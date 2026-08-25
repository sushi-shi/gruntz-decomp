# The EH unwind map is a C1 fingerprint: dead TRACEs allocate states and caller cb

tags: cpp:trace cpp:temp cpp:inline | asm:funcinfo | topic:codegen-idiom topic:wall
symptoms: `gruntz.delink.eh_band` reports a C1 state-count or map-topology
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

## Exhaustive current sub-100 assessment (2026-08-14)

Run:

```text
gruntz.delink.eh_band --nonexact \
  --nonexact-tsv /tmp/eh-nonexact.tsv
```

The audit joins all 827 current sub-100 scoreboard functions to the complete
750-group EH band and compares state count, every `(toState, has_action)` map
row, canonical funclet code, ordered teardown targets, and frame displacements.

| classification | functions | conclusion |
|---|---:|---|
| no retail EH map | 600 | pattern inapplicable; this is not a clean verdict |
| EH fingerprint identical | 192 | no destructible-scope, nesting, teardown, or frame-layout defect visible to this oracle |
| frame layout differs only | 26 | C1 structure and teardown agree; repair the owning frame/local model |
| C1 state count differs | 5 | structural scope or inline-cut defect |
| C1 map topology differs | 2 | equal `maxState` hid different scope nesting |
| teardown target differs | 1 | object/destructor/delete identity or inline-cut defect |
| candidate map missing | 1 | current candidate does not emit the retail EH map |

Two count differences have more candidate states than retail:
`CDDrawSurfaceMgr::RestoreChildren` and `CPlay::LoadGameAssetNamespaces`.
Three concurrently reconstructed status-bar
builders currently have fewer states: `CStatusBarMgr::BuildGameMenu`,
`BuildTabzDialog`, and `LoadTabSprites`. Their bodies and call sets must decide the
missing or surplus scopes and inline cuts. The equal-count topology differences are
`CGruntzMgr::PlayMovieEntry` (7 of 11 rows) and
`CDDrawSurfaceMgr::SnapshotChildren` (6 of 29 rows); their ctor/dtor inline cuts
nest differently. The remaining target-only
row is `CFontConfig::MeasureLabel` (`CPen` versus `CGdiObject` teardown).

`CButeMgr::SetString` formerly occupied a seventh count-difference row at 13/12.
Its exact standalone `CButeValue::CopyValue` body hid a C1 distinction: retail has
one return epilogue per switch body, while the source used per-arm `break` plus one
shared `return this`. Restoring per-arm returns leaves `CopyValue` byte-exact, closes
SetString's map to 12/12, and makes the other eight `CButeMgr::Set*` callers exact.

Two source corrections demonstrate why these categories outrank a local score.
`CGruntzMgr::Run` emitted a reconstruction-only
`CDDrawSurfaceMgr::operator delete`; removing it made constructor-failure cleanup
resolve the inherited `CObject::operator delete`, exactly as retail does, while
revealing the function's underlying frame-layout residue. In
`CChatBoxOwner::HandleTextInputKey`, the final status line was formatted into a
new `CString message`; retail's map proved that the already-live cheat-code string
was reused. Formatting into `code` removed exactly one state and made all 26 map
rows equal. The remaining `zPtrColl` guard funclets also have identical opcodes and
targets; after canonicalizing their `cmp [ebp+disp],0` and
`mov [ebp+disp],0` operands, they correctly classify as frame layout rather than
different teardown code.
`CRollingBall::Update` reused the arrival tile's original `tx`/`ty` for its later
arrow lookup; recomputing them after the collision switch shortened their lifetime,
changed behavior after a sink-edge target adjustment, and displaced both CString
funclets. `CGrunt::ResetEntranceAnimation` needed a lexical block around its tail
lookup so VC5 could place the CString in the dead `apply` parameter home. The latter
change is primary-code neutral: only the unwind row proves it happened.

Therefore the destructible-temp half of this pattern proposes **no remaining
source addition among current sub-100 functions**. The caller-cb half is not
exhaustively decidable from EH: a dead statement with no destructible temp can
change inline budgeting without leaving a map state. Require independent
call-set/inline-budget evidence before adding one. An identical fingerprint
does not prove the whole function correct, and the 600 mapless functions yield
no evidence either way.

Primary code exactness is also not proof. `CKeyedList::AddNode`, outside this
sub-100 census, formerly had exact primary code with only 2 of retail's 3 states.
The authentic release-elided `TRACE("%s\\n", node->GetName())` supplies retail's
NULL-action state and makes the full map exact; its longer-lived zero register
drops the primary score. Keep the structurally complete source rather than the
old exact-body local minimum. The all-function census also catches an analogous
inline-cut residue in exact-primary `CPixelTileImageSet::~CPixelTileImageSet`: retail expands the
empty `CTileImageSet` layer to `CObject::~CObject`, while the current TU calls the
implicit base destructor COMDAT.

## Reading the fingerprint safely

Each `new T` with a throwing ctor contributes a delete-guard state
(`??3@YAXPAX@Z`). An inline-expanded ctor chains its during-ctor states under
that guard in member declaration order. A funclet body's `add ecx,<disp>` names
the member offset. `??_M@YGXPAXIHP6EX0@Z@Z` with a pushed dtor callback proves
an array member `T[n]`; its arguments expose dtor, count, `sizeof(T)`, and the
member address.

Compare the full map, not only `maxState`: `PlayMovieEntry` proves equal counts can
hide different topology. `gruntz walls ehactions 0x8fab0` also proves that an
action-sequence difference is not automatically an authored cleanup defect:
retail calls the embedded playlist CArray ctor/dtor while base expands them,
so retail has a preceding-member CFecFile action during construction and uses
saved-receiver-plus-member-offset actions while decomposing destruction. The
tool reports this as `ACTION SHAPE DIFFERS`; adjudicate the relevant ctor/dtor
call boundaries and state map before changing source. Its decoder must retain
both direct `lea ecx,[slot]` receivers and `mov ecx,[saved]; add ecx,offset`
member receivers—the latter previously appeared incorrectly as `(no slot)`.

Compare the candidate COMDAT from the retail group's
owner TU: header-inline copies can have TU-specific C1 maps, and an arbitrary
duplicate falsely reported `CMoviePlayer::~CMoviePlayer` as 4 versus 5 even
though the owning `creditsstate` copies are 5 versus 5. Canonicalize both
`[ebp+disp8]` and `[ebp+disp32]` frame operands before comparing funclet code,
including register `mov`/`lea` and the immediate `cmp`/`mov` forms used by nested
construction guards. Otherwise a large frame shift looks like a different teardown
body. `HandleTextInputKey` is the integration control: its three `zPtrColl` guards
differ only in those displacements and must remain in the frame-layout class.

Costing caveat: a TRACE temp or named conversion may add a lifetime-flag
zero-init and flip cl's zero-register heuristic. VC5 passes an lvalue `CString`
through varargs bitwise, producing no temp or state. Dead string literals do
not enter the final string pool. Statically unreachable code after `return`
allocates no state because C1 drops it before state allocation, so it cannot
substitute for TRACE.
