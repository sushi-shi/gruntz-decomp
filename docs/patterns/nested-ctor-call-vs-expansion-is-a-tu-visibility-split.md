# Two entities, continued: the split need not follow the class, and NESTED ctors need a TU split

- **confidence** c10
- **tags** `cpp:ctor` `cpp:inline` `cpp:class` | `asm:call` | `topic:codegen-idiom` `topic:wall`

[two-shapes-need-two-entities.md](two-shapes-need-two-entities.md) establishes the recipe: a
body retail both `call`s and expands is TWO source entities, separated for a ctor by a tag on
the **inline** sibling. This is what applying it to the two remaining base ctors added -
`??0CUserLogic@@QAE@PAUCGameObject@@@Z` (0x58cd0) and
`??0CGameObject@@QAE@PAVCDDrawSurfaceMgr@@HH@Z` (0x15b390), both **0 -> 100.00 EXACT**, and the
same split on `??0CMotionState@@QAE@XZ` (0x136d0) taking `CGrunt::CGrunt` 60.95 -> 90.40 and
`CProjectile::CProjectile` 60.54 -> 86.23.

## A big body needs a shared helper, because MSVC 5 has no delegating ctors

The two ctors cannot call each other. Put the body in **one inline member function** that both
expand, so there is still exactly one textual copy:

```cpp
class CUserLogic : public CUserBase {
public:
    enum EInlineBase { INLINE_BASE };
    CUserLogic(CGameObject* obj);                 // pinned, out of line at 0x58cd0
    CUserLogic(CGameObject* obj, EInlineBase);    // the expanded sibling
    void AttachToObject(CGameObject* obj);        // the one copy of the body
};
inline void CUserLogic::AttachToObject(CGameObject* obj) { ...405 bytes of body... }
inline CUserLogic::CUserLogic(CGameObject* obj, EInlineBase) { AttachToObject(obj); }
```

The helper layer is not free but it is *favourable*: cl declines to expand a 400-byte inline in
some TUs, which is the behaviour retail shows. What differs **per entity** goes in the
**mem-init list**, which is per-ctor - that is where one ctor seeds a member inline and its
sibling leaves the member's ctor a `call`, with the body still shared.

## The split need not partition by class

For 0x58cd0 it happens to: the three retail callers all arrive through `CMovingLogic` or
`CDoNothingNormal`, so 56 initialiser lists take the tag and two do not.

For 0x15b390 it does **not**. `CWwdSpriteObject` (0x1dc from `operator new`) is created at three
sites - `CreateSpriteObject` **expands** the base, `ReadPlaneObjects` and
`CWwdGameObject::CreateObject` **call** it. So `CWwdSpriteObject` needs *both* ctors and the
`new`-site picks:

```cpp
new CWwdSpriteObject(OwnerMgr(), id, objectFlags, CGameObject::INLINE_BASE);  // expands
new CWwdSpriteObject(OwnerMgr(), id, 0);                                     // calls 0x15b390
```

Read the call site to be sure it is the *base* and not the derived ctor: at 0x162bad the
`call 0x15b390` is followed by `lea esi,[ebx+0x1a0]; call <CAniAdvanceCursor ctor>`, the
`[ebx] = ??_7CWwdSpriteObject` stamp and the five +0x18c..+0x19c stores - the derived ctor is
expanded *around* a called base.

## The ctors NESTED inside the pinned body: a TU split, not an inline

0x15b390 also **expands** `??0CResolveNode@@QAE@...` and `??0CLogicRecord@@QAE@...`, while the
three factories that expand `CGameObject`'s own body still **call** both. The obvious reading is
an inline-depth rule and the obvious fix - make those two ctors `inline` in their own headers -
is **wrong**: cl 5 then expands them at *every* site, both COMDATs vanish from every base obj
(`llvm-nm build/objdiff/base/*.obj` finds zero emitters) and the labels ratchet fires with no
unit gaining them.

The lever that works is **per-TU inline visibility**, the device already shipping as
`LogicTypeTableInline.h` / `*DtorInline.h`:

- the ctor stays **declaration-only** in its normal header and **out-of-line in a TU that must
  call it** - here `WwdObjMgr.cpp`, whose three factories are the retail call sites;
- a one-body `include/<Module>/<Class>CtorInline.h` defines it `inline`, and **only** the TU
  that expands it (`WwdFactoryObject.cpp`) includes that header.

The same question recurs one level down and answers differently there. `m_region`
(`WwdRegion`) and `m_shadow` (`WwdDirtyRect`) are *members*, so no header split is needed: the
pinned ctor names `m_region(WwdRegion::INLINE_SEED), m_shadow(WwdDirtyRect::INLINE_SEED)` in its
mem-init list and the tagged sibling says nothing, taking the pinned 0x15b2b0/0x15b270 bodies.
**Rule of thumb: a member sub-object is steerable from the mem-init list; a base sub-object or a
free call inside the shared body is not, and needs the TU split.**

## Two housekeeping consequences

- **tu_order** measures a unit's span from its `RVA()` rows only, not its `RVA_COMPGEN` pins.
  Homing `CResolveNode`/`CLogicRecord` at 0x15b2c0/0x15b300 into `wwdobjmgr` is free (its span
  already ends at 0x15b340); giving `??0CMotionState@@QAE@XZ` a real `RVA()` claim at 0x136d0 in
  `SerialObjectFactory.cpp` is not - it stretches that unit across the carved
  `SerializeSyncMarker` (0x13610) and adds an interleave pair.
- moving an out-of-line body between units is a **transfer**, not a loss
  (`wwdfactoryobject 57 -> 55`, `wwdobjmgr 52 -> 54`); the function census
  (`config/retail/gruntz_functions.tsv`) keys on RVAs, so it passes on its own -
  check the rows really re-homed rather than vanished.

## What this supersedes

`base-ctor-pinned-out-of-line-costs-every-derived-ctor.md` ruled "never introduce an
out-of-line definition to farm the one row"; with the tag split there is no trade.
`inline-base-ctor-emission-wall.md` recorded 0x58cd0 as unclaimable; it is claimed.

related: [two-shapes-need-two-entities.md](two-shapes-need-two-entities.md),
[base-ctor-pinned-out-of-line-costs-every-derived-ctor.md](base-ctor-pinned-out-of-line-costs-every-derived-ctor.md),
[inline-base-ctor-emission-wall.md](inline-base-ctor-emission-wall.md),
[inline-visibility-splits-call-and-expansion.md](inline-visibility-splits-call-and-expansion.md)
