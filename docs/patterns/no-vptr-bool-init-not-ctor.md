# A polymorphic initializer with no vptr store and a boolean result is a method, not a constructor

tags: cpp:ctor cpp:method cpp:virtual cpp:inheritance | asm:mov asm:ret | topic:codegen-idiom topic:identity
symptoms: a supposed polymorphic constructor never writes a vptr; success returns eax=1, failure eax=0; callers immediately test eax and preserve a derived vtable
confidence: 10/10
variants: vptr-stamp-void-init-not-ctor.md, inline-base-dtor-folds-into-leaves.md

For a standalone routine on a polymorphic object, these properties jointly
disprove a constructor model:

- there is no write to `[this]` anywhere in the routine;
- success explicitly returns `1` and failure returns `0`, rather than returning
  `this`;
- callers immediately test `eax`;
- callers construct a derived object first and expect its vptr to survive.

The routine is an ordinary initializer such as `Setup`, usually inherited by
the derived class. Modeling it as a base placement-constructor invents a base
vptr re-stamp, forces an abstract base to be declared concrete, and tends to
spread layout casts through allocation and destruction sites.

## Evidence

`CWwdGrid::Setup(RECT, int, int)` at 0x1915c0 has two exits: `eax=0` after a
failed bucket allocation and `eax=1` after setting the allocated flag. It never
stores either grid vtable. `CWwdSpatialMgr::Init` first stamps
`CWwdGridShell`'s vtable at 0x1f0310, then calls 0x1915c0 three times and tests
each result. The base vtable at 0x1f0328 contains `__purecall` for `OnFound`;
the derived vtable overrides it.

The false constructor model required `reinterpret_cast` assignments,
placement-new over live derived objects, and a declared-but-undefined concrete
`CWwdGrid::OnFound`. It scored 74.15% for 0x1915c0 and about 92% for the caller.
Restoring an abstract base plus a normal inherited `Setup` raised the functions
to 79.55% and 99.51%, removed the declared-only symbol, and enabled the adjacent
`Setup(RECT)` overload to be reconstructed.

## Reverse use

Search low-matching reconstructed constructors for the signature above:
boolean exits, no vptr store, and callers that test the return. Reclassify those
as methods before attempting register-order steering, then remove any
placement-new, casted `this`, or fake concrete virtual declaration introduced
to support the constructor story. Confirm the repaired hierarchy independently
from base/derived vtables and destructor vptr transitions.

Do not apply this from the missing vptr store alone: a non-polymorphic
constructor has none, and an inlined base constructor's dead vptr store may be
eliminated beneath a derived stamp. The explicit boolean result in the
standalone body is the decisive discriminator.
