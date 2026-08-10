# An unwind funclet NAMES the type of the object it destroys - it is a member/base oracle
tags: cpp:eh cpp:ctor cpp:dtor cpp:class | asm:jmp asm:call | topic:identity topic:correctness
symptoms: a wrapper struct with one member; an invented intermediate base class; a class-level `operator new`/`operator delete` that only forwards to the global; a `.cpp`-pinned function whose only retail callers are unwind funclets; `python -m gruntz.audit.eh_band --census` reporting `different-targets`
confidence: 10/10

## The oracle

cl 5.0 gives every `/GX` function an unwind funclet per EH state, and each one is
two or three instructions that say exactly what has to be destroyed and how:

```
mov ecx,[ebp-X]           ; the object
add ecx,0x18              ; ... this sub-object of it
jmp ??1SomeClass@@UAE@XZ  ; ... is destroyed BY THIS FUNCTION
```

The DESTRUCTOR IT NAMES is a fact about retail's declaration, and nothing else in
the binary states it as directly. The constructor call site cannot: a wrapper
around one member and the member itself call the SAME ctor at the same address, so
the two are byte-identical there. Only the destructor path tells them apart,
because a wrapper gets its own compiler-generated `??1Wrapper@@QAE@XZ` COMDAT that
calls the member's, while the plain member is destroyed directly.

`python -m gruntz.audit.eh_band --census --top N` reads both sides' funclets out of
the normalized objects, masks the relocation operands and histograms
`ours -> retail` over the ones that disagree. One row there is ONE modelling
decision, however many TUs repeat it.

## What it caught (2026-08-10, 146 of 750 groups)

* **57x a wrapper that was a member.** `CUserBaseLink` held exactly one `zBitVec`
  and an empty destructor. Every `CUserLogic`-derived ctor's funclet for `this+0x18`
  jumps to `zBitVec::~zBitVec` in retail and to the wrapper's synthesised COMDAT in
  ours. Its only out-of-line function - `CUserBaseLink::CUserBaseLink()` at 0x16d710,
  `{}` with the header's `inline zBitVec::zBitVec()` expanded into it - IS
  `zBitVec::zBitVec()`. Dissolving the wrapper fixed 124 funclets, and the
  frame-offset class fell 105 -> 41 at the same time: the wrapper was shifting the
  frame in those constructors too.

* **34x + 13x a mis-attributed compiler helper.** A funclet is often the ONLY caller
  of a helper, which makes its caller set diagnostic. `?RezFreeStdcall@@YGXPAX@Z`
  at 0x853d0 had 56 callers and every one was an unwind funclet - no game code at
  all - with the body `void __stdcall f(void* p) { ::operator delete(p); }`. That is
  MFC's `CObject::operator delete`, and the game function was fabricated.
  `?Tm_DestroyArray@@YGXPAXHIP6AXXZ@Z` at 0x11f640 was a `reloc-alias` guess sitting
  at HIGH confidence over the anchored `??_M@YGXPAXIHP6EX0@Z@Z` at LOW - the body
  walks an array backwards calling `[ebp+0x14]` with ecx = element, which is cl's
  vector destructor iterator exactly.

* **8x a class-level `operator new`/`operator delete` that only forwards.** Retail's
  funclet calls the GLOBAL `??3@YAXPAX@Z`. A forwarder cannot produce that: cl
  inlines it at the call sites but emits `??3CSymTab@@SAXPAX@Z` for the funclet to
  call. So a funclet naming the global REFUTES the class-level operator.
  (CSymRec, CSymTab, PureSoundElem.)

* **18x an empty destructor that still needs an ADDRESS.** `~WwdRegion() {}` /
  `~WwdDirtyRect() {}` are inlined away at every call site - retail's `~CGameObject`
  has no call to either - but their funclets jump to a real 16-byte function
  (`c3` + cl's 0x90 COMDAT padding) at 0x15b4e0 / 0x15b290. The fix is an
  `RVA_COMPGEN` pin on the COMDAT, NOT an out-of-line body: moving the body puts a
  real `call` in every destructor that retail inlines away.

## Reading the census's other two classes

`frame-offset` (same shapes, same destructors, different `[ebp+disp]`) says our
LOCALS are laid out differently - an extra or missing local, or a different
declaration order. `permuted` (the same funclets in a different order) says we
construct the same objects in a different SEQUENCE. Both are statements about
locals that no other view exposes, and neither is a wrong type.

`--check` additionally compares our `maxState` against retail's straight out of
both `FuncInfo` records. That is the strongest claim the band makes - a difference
means our function builds a different NUMBER of destructible objects - and it sees
cases the funclet list cannot, because two states unwinding one object share a
funclet address and collapse into one entry (11 groups diverge where the
funclet-count class found 3).

## Caveat

Compare RESOLVED ADDRESSES, not names, wherever both sides resolve. A name only one
side has ever heard of is usually a COMDAT our tree emits and retail placed
elsewhere - informative, but not the same finding as two different addresses.
