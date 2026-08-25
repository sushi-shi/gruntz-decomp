# The vtable-realizer's span adjudicates header-inline vs out-of-line per BODY

**Question it answers.** A small virtual body (GetTypeTag / SerializeDispatch /
Update / a dtor-adjacent Reset) could have been spelled in-class in the header
or out-of-line in the class's `.cpp`. Both compile to identical bytes, both are
`call *(vtbl)`-dispatched (MSVC5 never devirtualizes), so the score cannot
decide. Retail PLACEMENT can, body by body.

**Mechanism** (first-definer-wins, `docs/link-text-layout.md`): a header-inline
member is emitted as a COMDAT by every TU that realizes the class's vtable, and
link 5.10 keeps the copy from the FIRST such obj on the link line. An
out-of-line `.cpp` member is emitted exactly once, inside its own obj's run.
There is no /OPT:ICF and distinct symbols never fold, so "byte-identical across
N classes, folded to first use" - the wording of the retired `@interleaver`
claims - is not a mechanism that exists.

**The test.** Find an obj EARLIER on the link line than the class's own obj that
provably realized the class's vtable (its span holds the class's kept `??_G` /
`??1` / an inline ctor / another kept virtual of the same class). Then, for each
remaining virtual body of that class:

* retail copy sits inside the REALIZER's contribution -> the body was visible
  to the realizer -> **header-inline**; spell it in-class with its `RVA()` pin
  (the AniCycle.h / PathHazard.h GetTypeTag precedent).
* retail copy sits in the class's OWN run although the earlier realizer kept
  that class's other COMDATs -> the realizer had no definition to emit ->
  **out-of-line in the .cpp**; leave it there.
* the body has ZERO refs anywhere (no rel32 caller, no data slot, no
  address-taking - `sema xref --tree`) -> it cannot be an inline at all (an
  inline never odr-used is never emitted; only /OPT:REF-less linking kept it)
  -> out-of-line **in the obj whose run contains it**, wherever the current
  tree homes it.

**Worked examples (2026-08-13 interleaver re-adjudication).**
serialobjectfactory.obj (0xd210-0x13c5e, early on the line) realizes ~45 logic
vtables - its run holds their kept inline ctors (`??0CTileTrigger` 0x11160,
`??0CProjectile` 0x126e0) and `??_G/??1` groups.

* `CAniCycle::SerializeDispatch` kept at 0xf470 INSIDE that run -> header-inline;
  moved in-class, rebuilt 100%, attribution follows the includer. Same for 27
  sibling bodies (SerializeDispatch x13, GetTypeTag x9, CMovingLogic::FinalizeStep
  at the run's deferred-inline tail 0x13c70, the CPlay trio + CAttract::Update
  in gruntzmgr's state band, CSBI_WellGoo::Reset in the statusbar compiland's
  dtor cluster).
* Negative control: `CGruntStaminaSprite::GetDisplayedValue` sits at its OWN
  obj's tail (0x7fbb0) although serialobjectfactory kept that class's
  GetTypeTag (0x12020) and `??_G` (0x12040) - the factory realized the vtable
  but had no GetDisplayedValue definition to emit -> retail spelled it
  out-of-line in the `.cpp`. Ditto the toy/wingz sprites.
* Zero-ref case: `CPreviewState::LoadScreen` 0xfab90, no refs at all, mid-run
  in attract's span -> out-of-line in Attract.cpp (re-homed there; the
  levelpreview unit loses its phantom second span).

**Traps.** Do not move a non-virtual multi-caller body to a header
(`nonvirtual-inline-header-craters-delinker-packing.md` - MSVC5 inlines it into
every caller). Direct-called statics (RegisterActs, called by
gameobjectlogictypes) stay out-of-line for the same reason regardless of
placement. And run the test per BODY, not per class: one class routinely mixes
both spellings (GetTypeTag inline, GetDisplayedValue out-of-line).

**MEASURED LIMIT (2026-08-13): expansion evidence outranks placement.** The
test must NOT be run in the inline->out-of-line direction on placement alone.
`??1CUserLogic` (kept at 0x8860 in userlogic's run although 46 of our base
objs emit it and actionarea links first) looked like an out-of-line user dtor;
converting `~CUserLogic() {}` to out-of-line cratered ~70 derived dtors to
2.35% - retail's derived `??1` bodies inline-EXPAND the base dtor, which only
an inline-visible spelling produces. Check callers/derived bodies for
expansions FIRST; any expansion pins the inline spelling, and the placement
anomaly then signals an unresolved partition (who is the true first
materializer / is the "host" run really a separate obj), not a spelling.
Corollary: cl 5.0 materializes standalone `??1`/`??_G`/`??_7` copies far more
selectively than "every referencing TU" - a ctor expansion that STAMPS
`??_7CUserBase` does not force that TU to emit the vtable family.

**Labeling limit.** An `RVA()` claim on a header body binds only if clang
EMITS the body in some labelling TU (the empty-fragment gate names this).
Virtuals always qualify (vtable slot reference); a pure accessor clang inlines
everywhere does not (`CMenuItem::Reset` stays a pin). The 148 `??1` pins
(65 header-spelled dtors materialized out-of-line, 83 implicit) and 20 `??0`
pins (19 header-spelled inline ctors) survive for this reason - the pin names
the compiler-materialized copy, while the spelling, when one exists, already
lives in the header.
