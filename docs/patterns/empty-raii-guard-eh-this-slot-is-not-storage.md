# An empty RAII guard's EH `this` slot is not source storage

tags: cpp:eh cpp:class cpp:dtor | asm:lea asm:call | topic:wall topic:eh
symptoms: primary code has the exact guard ctor/dtor expansion and frame size; unwind
action and target agree, but the funclet's `lea ecx,[ebp-N]` differs by an arbitrary,
sometimes very large displacement; the guard has no data members and its destructor
does not read `this`
confidence: 10/10

## Why the displacement is not an object-layout oracle

An inline RAII guard can be an empty class whose constructor and destructor only call
global/application services. C1 must still describe a destructible scope and emit an
unwind action, but no normal-path instruction ever needs an address for the object and
the destructor does not consume its receiver. The funclet's synthetic `this` address
can therefore be colored onto any dead frame home. Its displacement is not evidence
for a missing array, buffer, or padding local.

Gruntz's wait-cursor guard is the calibration case. Retail directly expands
`AfxGetModuleState` plus `BeginWaitCursor`/`EndWaitCursor`; using MFC 4.2's real
`CWaitCursor` would instead introduce its out-of-line ctor/dtor calls. The project
inline guard is therefore the correct call shape, and it has no fields.

## Controls

- `CGruntzMgr::ResetWorldState` (`0x91e20`) has a 100% exact primary body, exact
  16-block/11-branch/3-return shape, and the exact normal Begin/End expansion. Its one
  unwind receiver differs by `+0xc`.
- `CBattlezDlgCustom::DoDataExchange` (`0x180e0`) has the exact `0x524` local
  reservation and exact 14-block/8-branch shape, yet its receiver differs by
  `-0x514`. Its two `CString + const char*` sites have a separate call/expand
  difference; that does not give the fieldless guard storage.
- `FillCustomLevelList` and `StartUpPrompt` show `-0x11c` and `-0x8` respectively.
  `StartUpPrompt` is now a stronger control: its two in-scope returns reproduce
  both retail destructor exit copies and make the complete primary body exact,
  while the synthetic guard receiver remains eight bytes away.

The differing magnitudes with the same fieldless guard disprove a real object-size
interpretation. Require matching scope count, teardown code, teardown target, and the
guard's Begin/End callee identity. Classify only this receiver displacement as bounded
C1 frame coloring; independent inline or exit-copy differences in the owner remain
real work and must not be hidden by this bound.
