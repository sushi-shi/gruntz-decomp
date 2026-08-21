# MSVC5 schedules statements faithfully: call-crossing locals and declaration position determine the ready-list order
tags: cpp:local cpp:win32 asm:mov asm:push | topic:codegen-idiom topic:scheduling
symptoms: a single byte/instruction shifts when a store moves relative to a call; a value is repeatedly reloaded from an aggregate where retail gives it a separate stack home; independent scalar loads have retail's registers but the wrong order
confidence: 9/10

At /O2 the visible instruction order tracks the SOURCE statement order more tightly than expected.
Ten corollaries, all steerable or diagnostically bounded by source lifetime/position:

- **A store emitted BETWEEN an arg push and its `call`** must be written *before* the call
  statement in source (moving it after the call shifts a byte). E.g. CNetMgr's shared-flag store
  `g_sharedFlag=0` lands between the `sub` and the first `je` → write it right after the dispatch.
- **Member inits emit in the optimizer's schedule order, not declaration order** in some ctors
  (CGameApp stores +0x10 before +0x0c) but flat-scalar ctors ARE declaration-order-faithful
  (CState ctor matched first try). Mirror the order you read from the dump.
- **A local string-literal array's DECLARATION POSITION drives scheduling**: declare
  `char s[]="Software";` JUST BEFORE its use, not at function top — a top declaration scheduled
  the `.data`/`.rdata` 3-load copy idiom early and broke ~15%. Read m_250 up front when the
  target hoists its load (ShowError: reading both error members early kept the m_250 load live
  across the id-default; lazy read floated it below → DIFF_DELETE/INSERT pair).
- **A member copied to a named local before several calls is a distinct call-crossing
  value.** If retail loads `ps.hdc` once, writes a second stack home, and reloads that
  home for both drawing calls while the base repeatedly reads `PAINTSTRUCT::hdc`, write
  `HDC hdc = ps.hdc;` and use `hdc`. The extra home is semantic lifetime evidence, not
  frame noise.
- **Declaration order is the final tie-break for independent ready computations.** Once
  register roles and instruction counts agree, move the short scalars whose loads lead in
  retail before their siblings. In `LevelPreviewDlgProc`, declaring `dx`/`dy` before
  `w`/`h` changed only the ready-list order of the rectangle/point loads and closed the
  remaining exact-size residue.
- **A saved global followed by a conditionally changed current value is a post-write
  reread, not two locals initialized together.** Retail's `load current; copy saved; test
  saved`, followed by a possible global store and a later test of `current`, comes from
  snapshotting the global, performing the conditional store, and only then reading the
  global into the current-value local. VC5 eliminates the reread while preserving the
  saved/current split. Initializing both locals before the guard leaves the same dataflow
  but schedules `test current` before `copy saved`.
- **A trivial inline accessor is also a scheduling boundary.** For a member chain used as
  a `this` argument, spelling `m_world->m_drawTarget->Call(a, b)` lets cl sink both member
  loads below the argument pushes. Spelling the real shared accessor
  `menuRoot()->m_drawTarget->Call(a, b)` emits the accessor's `m_world` load before the
  pushes and the `m_drawTarget` load after them. Do not retain a one-site local when an
  established accessor produces the same lifetime; neighbouring callers corroborate the
  source boundary.
- **Two independent member assignments retain their authored priority even when arithmetic
  is interleaved between them.** If retail finishes an X-coordinate expression before
  storing an adjacent delta, write the X assignment first. In
  `CSBI_SideTab::BuildStatzTabStatusBar`, moving `m_drawPosition.m_x` before
  `m_bottomFrameDy` in both arms made cl keep the delta constant live while completing the
  coordinate arithmetic, reproducing retail's EDX/ECX schedule and raising 80.1866 to
  90.0746. The object extent became exact; the remaining exit placement was independently
  closed by the partial `goto fail` regime.
- **Values loaded before an intervening lookup are source-level argument lifetimes, and their
  declaration order fixes their register roles.** If a switch arm loads every later constructor
  argument before a map lookup, name those values before the lookup rather than re-reading the
  record at the call. Order the declarations by the retail register assignment, then preserve
  the retail fall-through arm. `CDDrawChildGroup::LoadObjects` preloads `sortKey,id` before its
  deferred-object lookup and `sortKey,y,x,id` before both nullable worker lookups; writing the
  null case first reproduced the two retail branch exits. Together with initializing the shared
  result before the switch, this changed 218/21 instructions/branches into 231/23 against
  retail's 236/23 and raised 80.2619% to 96.88%. Calls, returns, and all ordered referents remain
  exact. A 32-state compiler forest was one flat island; 256 depth-1/2 structural variants found
  no improvement, so the five-instruction residue is not a declaration merge, hoist, relation,
  commutative order, or TU-state lever.
- **A loop-entry jump that skips one reload can be a call-crossing lifetime consequence, not a
  different loop construct.** In `CDDSurface::ShadeRect`, retail computes the scaled percentage
  before `Lock`, so `new[]` returns the scratch pointer in EAX and the first iteration of each row
  loop jumps over the latch-only `mov eax,[scratch]`. The current compile sinks the percentage
  calculation below `new[]`, consumes EAX, and therefore reloads scratch on the first iteration;
  both forward jumps disappear. Computing the final LUT offset before `Lock` is the causal
  control: it restores 28/28 branches and both skip-reload jumps, but changes the frame from
  retail's 0x1c to 0x20 and grows the body to 0x2dc, proving that the final offset is the wrong
  call-crossing value. Treat the jump pair as a live-range signature and reconstruct the value
  that crosses the call; do not rewrite a correct `for` loop into a synthetic rotated loop.

STEERABLE. Evidence: CNetMgr::OnOutOfSync flag interleave; CState ctor (decl-order, byte-exact)
vs CGameApp ctor (schedule-order); GetGruntzDriveLetter `"Software"` local; CGruntzApp::ShowError
m_24c/m_250 hoist; `LevelPreviewDlgProc` 0x000e3690, 95.35% -> 98.07% from the
`HDC` home and then **100.00% exact** from `dx`/`dy` declaration order. The deeper
EH-state-write scheduling over CString live ranges is a WALL — see
eh-state-numbering-base.md / makerezpath residue.

`CMenuState::StartMusic` @0x0a05a0 is the saved-global control. The eager two-local
spelling stopped at 94.76%; reversing the local declarations fixed the register roles but
left `test current; copy saved` at 95.12%. Moving the current-value read below the
conditional global write emitted retail's `copy saved; test saved` and reached **100.00%
exact** with the same 0x74-byte CFG and all six relocation referents unchanged.

`CFaderFlat::RenderFrame` @0x17f660 supplies the dependent-local form. Declaring
`base = h - frame - 1` before `span = m_percent * h / 100` moves the parameter load
ahead of the height/width coloring, delays the first divergence from `+0x22` to
`+0x2c`, and raises the function from 90.07% to 90.79%. The controls distinguish the
source ordering from a register-name coincidence: putting `base` between `h` and `w`
falls to 88.09%, while declaring `w` before `h` reaches only 90.77%. A 32-island
TU-state forest found three states (best 93.47%), but changed only commutative address
arithmetic and spill-slot choices; none changed the remaining height/width/span
coloring or removed its two extra instructions.

`CPreviewState::LoadScreen` @0x0fab90 supplies the inline-accessor form. Replacing both
repeated `m_world` chains with the existing `menuRoot()` expansion moved the first world
load across the two `LoadPageImage` argument pushes exactly as retail does and raised the
function from 96.39% to 99.69%. Both sides remain 64 instructions, 4 calls, 6 branches,
6 returns, and 5 ordered referents. The sole residue is the already-bounded scratch
register choice on `frontPair->m_surface`, shared with adjacent `CState::RunTitle`.

`CDDrawChildGroup::LoadObjects` @0x15ad30 supplies the pre-lookup argument-lifetime form.
The retail sprite/container arms load four snapshot fields before `CMapStringToOb::Lookup`,
then use those values after the call. Keeping the field expressions at the constructor call
lets VC5 delay all four loads and leaves the function 17 instructions short. Semantic locals
recover the live-across-call set; their order is observed directly from retail's register loads,
not selected by score. The negative worker guard is independently required: the positive form
keeps the same meaning but chooses the opposite physical fall-through and loses the retail CFG.

`CDDSurface::ShadeRect` @0x13f460 supplies the skip-reload diagnostic form. Base and retail agree
on seven calls, two returns, twenty ordered relocations/addends and all format/LUT semantics, but
the base has 26 branches/0x2ca against retail's 28/0x2da. A 32-island campaign found eleven target
states: the 80.3591% state only fixes the RGB555 partial-register/table-load schedule and leaves
the 26-branch topology unchanged. A second campaign covered all 71 atomic source mutations
(72 shapes crossed with a state control); none improved or changed topology. Named scaled locals,
split multiply/divide, an in-place post-allocation shift, and an inline scaling helper are also
byte-inert at the decisive boundary. Only the deliberately over-live final-offset control moves
the calculation before `Lock`; its wrong frame and register set bound the remaining gap as the
compiler's choice of call-crossing value, not missing row-loop logic.
