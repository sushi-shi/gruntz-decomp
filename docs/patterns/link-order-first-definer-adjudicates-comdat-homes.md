# Link-order FIRST-DEFINER adjudicates a kept-COMDAT body's home (and the ASSERT-cb lever does NOT break the inline exemption)

Class: layout / identity. Proven 2026-08-15 during the tu-order baseline drain
(the 38-row kept-comdat-exiles ledger -> 0).

## The rule

link.exe 5.10 keeps a multi-defined COMDAT from the FIRST obj on the link line
that defines it. So for a body whose retail copy sits inside obj K's
contribution:

* Every retail CALLER in an obj EARLIER than K proves that caller's TU did
  NOT define the body (had it defined it - as any TU that sees a header-inline
  definition and declines to expand it does - the copy would sit in THAT obj's
  contribution, before K). The body was not header-visible there: it is an
  OUT-OF-LINE definition in K's compiland, called as an ordinary external.
* Only when the earliest referencing obj IS K can the header-inline model
  stand (K materialized first). `also_units` + the link_order band flip then
  attribute the Model row to K automatically.

Under /O2 (which implies /Ob1) this is reinforced from the compiler side: an
`inline`-marked body with front-end cb <= 0x28 is budget-EXEMPT - cl expands
it at every call site, so it can never produce image-wide out-of-line calls.
Retail calls at every site => the callers never saw an inline definition.

## Worked examples (all from the drain, evidence in-session)

* `?SetParams@CMotionState@@QAEHNNNNNNNNNNN@Z` (0x58bc0), `?SetZ` (0x58ca0),
  `??0CUserLogic@@QAE@PAUCGameObject@@@Z` (0x58cd0): kept inside
  gruntcombat.obj (seq 68), but retail callers include `??0CGrunt` in
  grunt.obj (seq 63) via ILT thunks 0x2ccf/0x3ea9/0x3828. 63 < 68 => grunt
  never defined them => out-of-line definitions in the retail combat TU. Homed
  in GruntCombat.cpp; `??0CUserLogic` rebuilt to 100.00 there once the TU also
  carried MotionState.cpp's two .inl devices
  (`LogicRecordRegistryFindInline.h`, `LogicTypeTableInline.h`).
* `?IndexToPtr@_zdvec@@QAEPADH@Z` / `_zvec` (0x310f0/0x312a0): converting them
  to ZVec.h header inlines made cl expand them at ~22 sites inside
  `RegisterGruntActions` (base 0x138d B / 156 calls vs target 0x9e5 B / 113
  calls, each expansion dragging `GetRetAddr`/`GrowTo` calls in; 99.4 -> 2.1).
  Retail CALLS them there => not header-visible; both homed out-of-line in
  BattlezMapConfig.cpp's tail pocket (the keeper, band 0x24dc0-0x31314).
* `?GetTileHandle@CDDrawWorkerHost@@QAEHHH@Z` (0xd53a0): callers play (139),
  tileswitchlogic (199), gamelevel (249). First caller IS the keeper (play) -
  but the header-inline model still fails, because cl auto-inlines the
  0x19-byte body everywhere and no obj materializes a copy at all (the claim
  binds nowhere). Out-of-line in Play.cpp at its band position.

## Negative control: the compiled-out ASSERT does NOT lift the exemption

Titrated on GetTileHandle with the real cl 5.0 (probe TU + tileswitchlogic):
1x `ASSERT(...)`, 4x `ASSERT(...)` (MFC release form `((void)0)`), and an
`if (0) { return -1; }` dead branch all left the accessor exemption-inlined -
no COMDAT emitted, callers still expand it. The ASSERT-cb lever is a
budget-REGIME tool (bodies already near/over cb 0x28 whose callers must
decline); it cannot make a tiny accessor call-preserving. If retail calls a
tiny accessor out-of-line image-wide, the accessor was NOT inline-visible -
home it out-of-line; do not stack dead statements.

## Two companion devices proven in the same drain

* A header dtor must not carry `RVA()`: clang attaches the annotation to the
  destructor DECL, so the claim arrives for BOTH cl variants (`??1` and
  `??_G`) and collides with the real `??_G` pin (`??_GCWapObj` claimed at two
  rvas). Pin the kept `??1` copy with `RVA_COMPGEN` in the KEEPER's .cpp
  (Play.cpp holds `??1CWapObj`/`??1CImage`; DDrawSurfacePair.cpp holds the
  CAniRecord* dtors; MenuPage.cpp the CMenuItem* group).
* A class TU that retail shows keeping `??_G`/`??1` while its real ctor lives
  in another compiland realizes the vtable with the `Realize<C>()`
  `return new C();` device THROUGH THE HEADER-INLINE DEFAULT CTOR - `new` via
  an out-of-line ctor stamps nothing (DoNothing/BehindCandy/EyeCandy, whose
  CGameObject* ctors are FrontCandyAni.cpp's).

## How to run it

`gruntz sema xref <body-rva>` (then the ILT thunk) names the retail callers;
`link_order.tsv` seq-orders them against the keeper band. Caller earlier than
keeper => out-of-line at the keeper, full stop. First-caller == keeper AND the
body over the exemption => header inline is viable; verify the claim binds
(`gruntz labels --unit <keeper>`).
