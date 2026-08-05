# A derived destructor inlines its base/member destructor only if that one is defined IN the class

tags: cpp:dtor cpp:inline cpp:class | asm:call asm:mov | topic:codegen-idiom
symptoms: a derived `~CDerived` is a fraction of retail's length; where retail runs the base
  class's field-clearing stores and vptr stamps inline, your recompile has a single
  `mov ecx,esi; call <tgt>`; the EH state numbers in `[esp+N]` are lower than retail's (retail
  walks 1/3/4/8, you walk 1/3/2) because retail unwinds more inlined sub-objects
confidence: 9/10

## Mechanism

`/O2` implies `/Ob1`: cl 5.0 inlines only functions the source marked `inline` — which
includes any member function *defined inside its class body*. A destructor declared
`virtual ~CFoo() OVERRIDE;` in the header and defined out-of-line in a `.cpp` is NOT
inline, even when its definition precedes the use in the same TU. So the compiler-generated
`~CDerived` emits a `call ??1CFoo@@UAE@XZ` where retail, whose `~CFoo` was written in the
class, splices the whole base body (vptr stamp, member clears, its own base chain) in.

The same applies to a destructible *member*: `CWwdGameObjectA::m_animCursor`'s
`~CAniAdvanceCursor` is inlined into every owner's dtor only when it is an in-class body.

## The fix

Move the body into the class and pin the out-of-line copy with `RVA_COMPGEN` in the owner TU:

```cpp
// header
virtual ~CWwdGameObjectA() OVERRIDE {
    Unload();
}
// owner .cpp - the COMDAT still exists (the vtable references it)
RVA_COMPGEN(0x0015b790, 0x1a6, ??1CWwdGameObjectA@@UAE@XZ)
```

**Do NOT put `RVA(...)` on the in-class dtor.** A dtor defined in a header also emits
`??_G<Class>@@UAEPAXI@Z` (the scalar-deleting dtor) in every TU that instantiates it, and
`labels.py` then sees two symbols claiming one rva:

```
[labels] ERROR duplicate RVA 0x15b790: ... (??_GCWwdGameObjectA@@UAEPAXI@Z), ... (??1CWwdGameObjectA@@UAE@XZ)
```

`RVA_COMPGEN` in the single owner TU has no such ambiguity — the same device
`~CGameObject` already used.

Evidence: `CWwdGameObject::~CWwdGameObject` 48.2 -> 86.3 from the `~CWwdGameObjectA` move;
`CWwdGameObjectA::~CWwdGameObjectA` 86.8 -> 92.9 from the `~CAniAdvanceCursor` move.

## Caveat

The budget is finite and one-directional: making a member dtor inline can push a base dtor's
`Unload()` back OUT of the caller's inline budget (measured: `~CWwdGameObject`'s inlined
`CGameObject::Unload` reverted to a `call` once `~CAniAdvanceCursor` was spliced in). Each
level is an independent decision — read the retail dtor and match the levels it actually
inlined rather than assuming all-or-nothing.
