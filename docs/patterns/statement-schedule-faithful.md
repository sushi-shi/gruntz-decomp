# MSVC5 schedules statements faithfully: call-crossing locals and declaration position determine the ready-list order
tags: cpp:local cpp:win32 asm:mov asm:push | topic:codegen-idiom topic:scheduling
symptoms: a single byte/instruction shifts when a store moves relative to a call; a value is repeatedly reloaded from an aggregate where retail gives it a separate stack home; independent scalar loads have retail's registers but the wrong order
confidence: 9/10

At /O2 the visible instruction order tracks the SOURCE statement order more tightly than expected.
Five corollaries, all steerable by re-ordering/positioning source:

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

STEERABLE. Evidence: CNetMgr::OnOutOfSync flag interleave; CState ctor (decl-order, byte-exact)
vs CGameApp ctor (schedule-order); GetGruntzDriveLetter `"Software"` local; CGruntzApp::ShowError
m_24c/m_250 hoist; `LevelPreviewDlgProc` 0x000e3690, 95.35% -> 98.07% from the
`HDC` home and then **100.00% exact** from `dx`/`dy` declaration order. The deeper
EH-state-write scheduling over CString live ranges is a WALL — see
eh-state-numbering-base.md / makerezpath residue.
