# MSVC5 schedules statements faithfully: a store between an arg-push and its `call` must be WRITTEN before the call; local-array decl POSITION matters
tags: cpp:local asm:mov asm:push | topic:codegen-idiom topic:scheduling
symptoms: a single byte/instruction shifts when a store is moved relative to a call; a `.data`/`.rdata` 3-load string-literal copy is scheduled too early; member inits in the wrong order
confidence: 8/10

At /O2 the visible instruction order tracks the SOURCE statement order more tightly than expected.
Three corollaries, all steerable by re-ordering/positioning source:

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

STEERABLE. Evidence: CNetMgr::OnOutOfSync flag interleave; CState ctor (decl-order, byte-exact)
vs CGameApp ctor (schedule-order); GetGruntzDriveLetter `"Software"` local; CGruntzApp::ShowError
m_24c/m_250 hoist. The deeper EH-state-write scheduling over CString live ranges is a WALL — see
eh-state-numbering-base.md / makerezpath residue.
