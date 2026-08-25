# The ctor-vptr transposition breaks: the body's first statement is an inline member OF THE CLASS THAT OWNS THE RECEIVER

tags: cpp:ctor cpp:vtable cpp:inline cpp:member | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: a derived ctor stuck at 94-99.7% whose whole residue is that retail stamps `mov [this],??_7Leaf@@6B@` immediately after the last base-ctor store while cl emits it 1-4 instructions later, after the body's first `mov r,[this+N]`; instruction multisets identical
confidence: 10/10

## The mechanism

cl 5.0's list scheduler gives the body's first statement top priority (it starts the
longest dependency chain in the block) and hoists its RECEIVER LOAD over the leaf vptr
store, which has no successors. Retail does not hoist. The lever is an **inline-expansion
boundary**: an operation that lives inside an expanded inline member is not a candidate
for that hoist.

The boundary only helps if the RECEIVER LOAD ITSELF is inside the expansion. This is the
whole finding, and it is why every previous attempt failed:

| spelling | first body statement | result on `CWayPoint::CWayPoint` 0xae3f0 |
|---|---|---|
| written out | `m_wwdObject->m_stateFlags \|= SPRITE_STATE_HIDDEN;` | 99.673 |
| inline member on the **object** class (`CResolveNode::Hide`) | `m_wwdObject->Hide();` | 99.673 — unchanged |
| inline member on the class that **owns the pointer** (`CWapX::Hide`) | `Hide();` | **100.000 EXACT** |

In the middle row the load of `m_wwdObject` is still in the caller (it forms the `this`
argument), so it still hoists. In the bottom row it is inside `CWapX::Hide`'s expansion.

```cpp
class CWapX {
    CWwdGameObjectA* m_wwdObject;          // the receiver the ctor body reads first
    void Hide() { m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN; }
    void SetObjectFlags(i32 bits) { m_wwdObject->m_flags |= bits; }
    void ApplyName(const char* n) { m_wwdObject->ApplyName(n); }
    void ApplyLookupSprite(const char* n, i32 f) { m_wwdObject->ApplyLookupSprite(n, f); }
};

CWayPoint::CWayPoint(CGameObject* o) : CUserLogic(o, CUserLogic::INLINE_BASE), CWapX(o) {
    Hide();
}
```

A plain forwarder works as well as a real one-line mutator - what matters is only where
the receiver load sits. Corroboration that this is the devs' shape and not a fitted device:
the four members above cover 75, ~40, ~50 and ~45 textual sites across the tree, which is
the repeated-one-line-member signature of `repeated-container-call-is-an-inline-member.md`.

## Measured, one build each

`CWayPoint` 99.673, `CGuardPoint` 99.673, `CLevelTime` 99.673, `CSingleAnimation` 99.571,
`CWarpStonePad` 99.644, `CTileTriggerTransition` 99.561, `CGruntHealthSprite` 97.634,
`CGruntWingzTimeSprite` / `CGruntStaminaSprite` / `CGruntToyTimeSprite` 94.762,
`CGruntSelectedSprite` / `CStatusBarSprite` 96.458, `CGruntPowerupSprite` 96.304,
`CMenuSparkle` 96.421, `CGruntStartingPoint` 96.566, `CCursorSnapSprite` 96.634,
`CGruntToySprite` 97.800 — **all 17 to 100.000 EXACT**. Partial on the ctors whose residue
is more than the transposition: `CDoNothing` 99.521 -> 99.829, `CKitchenSlime` 99.576 ->
99.695, `CAniCycle` 94.242 -> 94.621, `CActionArea` 94.415 -> 96.547, `CExitTrigger` 93.593
-> 93.791, `CVoiceTrigger` 98.017 -> 98.317.

Negative control that pins the mechanism rather than the TU state: `CLevelTime` sat at
99.673 through the build in which `CWayPoint` and `CGuardPoint` (same TU state, same
header) reached EXACT, and only moved when its own first statement was converted.

## Second pass, 2026-08-16: nine more sites

Sites found by scanning every `CWapX`-derived ctor for a first body statement still
written out through `m_wwdObject`, converting only that statement:

`CBoomerang` 97.23 -> **100.000 EXACT**, `CExplosion` 92.30 -> 95.77 (`ApplyName`),
`CTeleporter` 92.02 -> 96.03, `CGruntVoice` 94.75 -> 97.58 (`ApplyName`),
`CActionArea` 96.55 -> 99.54 (`ApplyName`), `CTimeBomb` 88.21 -> 91.02,
`CPathHazard` -> 98.95, `CWormhole` 94.24 -> 94.54, `CGruntPuddle` 55.87 -> 56.17.
Sibling `CGameObject` destructors take the same lever from the other side —
[dtor-cleanup-writes-are-inline-members-that-pin-the-member-dtor-lea.md](dtor-cleanup-writes-are-inline-members-that-pin-the-member-dtor-lea.md).

## CHECK THE RESIDUE SIGNATURE FIRST - blanket conversion is inert

The lever fixes exactly one residue: a receiver load transposed with an adjacent
successor-less store (the leaf vptr stamp, or the leading run of member-init stores).
Read the first divergence before converting - dump both sides and look for
`base: mov r,[esi+N] | mov [esi],<vtbl>` against `target: mov [esi],<vtbl> | mov r,[esi+N]`
(or the same load hoisted over a run of `mov [esi+K],r` stores). Where the residue is
something else, the conversion changes NOTHING and only costs header ripple. Three
measured negative controls, one build each:

* `CUserLogic::SwapActKey(const char*)` for the 98-site pair
  `m_prevAnimSetNode = m_logicRecord->m_eventCode; m_logicRecord->m_eventCode = ActFindId("A");` -
  the strongest repeated-one-line-member signature in the tree - converted at 12
  first-statement sites (CSpotLight, CDroppedObject(+Shadow), CSingleFrameMessage,
  CTileTrigger, CBrickz, CCheckpointTrigger, CEyeCandyAni, CFrontCandyAni,
  CBehindCandyAni, CParticlez, CSimpleAnimation): **every one scored identically**,
  plus 7 fresh sub-bank rows from the `UserLogic.h` decl-count window.
* `CUserLogic::SnapToTileCenter()` for the 14-site `m_object->m_screenX/Y =
  (… & ~TILE_MASK_PX) + TILE_HALF_PX` pair: inert at CVoiceTrigger / CExitTrigger /
  CGruntCreationPoint (98.32 / 97.76 / 81.97 unchanged), 8 fresh sub-bank rows.
* `CWapX::SaveAnimation()` for the 121-site `m_value = m_wwdObject->m_animCursor.
  m_animation;`: at `CAniCycle`'s site - which is inside an `if`, not the ctor's first
  statement - it made things WORSE, 94.62 -> 93.23.

## When the WRITTEN-OUT form is retail's

Per-site, decided by cl's /Ob1 budget, exactly as in
`repeated-container-call-is-an-inline-member.md`: an expansion SPENDS ~cb where the same
statements written out FUND 2*cb. `CProjectile::CProjectile` 0xdec60 went 99.772 -> 41.411
when its (non-first) `Hide()` site was converted, and `gruntz walls diagnose` named the
cause directly — the call-target multiset changed, `?InitOwner@CMovingLogic@@AAEXABN@Z`
flipped from expanded to called and the body shrank 0x255 -> 0x147 bytes. Read a big drop
after this conversion as a budget flip, confirm it with `walls diagnose`, and write that
site out again.

**The FIRST statement is not exempt from that.** Re-measured 2026-08-16: converting
`CProjectile`'s FIRST statement (`m_wwdObject->m_flags |= 0x2000002` ->
`SetObjectFlags(0x2000002)`) craters it 96.91 -> 41.64 with the identical diagnosis
(`InitOwner` expanded -> called, 0x255 -> 0x147). Position decides which residue the
lever can fix; it does not decide whether the budget can afford the expansion. Measure
every site.

`CTileTriggerSwitch` 0x10dc40 is the control in the other direction: it is 100.000 EXACT,
its `m_flags |= 2` / `Hide()` statements are NOT first, and converting them left it at
100.000.

## Cost

Each new member is a declaration in a widely-included header, so it moves the /O2
decl-count window in every TU that includes it (`seams-stay-local-shared-headers-ripple`).
Four members in `UserLogic.h` produced 7 fresh sub-bank rows, six of them under 0.5 and one
at -7.0 (`?CenterOnGroup@CTriggerMgr@@QAEHH@Z`, whose residue is the TU-state-driven
commutative/memory-fold coin of `commutative-operand-order-is-canonical.md`). That is the
accepted structure-over-current-% trade; the banked MAX is preserved for all of them.

related: [vptr-stamp-transposed-with-second-base-member-load.md](vptr-stamp-transposed-with-second-base-member-load.md),
[derived-vptr-stamp-transposed-with-the-body-first-load.md](derived-vptr-stamp-transposed-with-the-body-first-load.md),
[eh-ctor-vptr-restamp-position.md](eh-ctor-vptr-restamp-position.md),
[repeated-container-call-is-an-inline-member.md](repeated-container-call-is-an-inline-member.md),
[vptr-stamp-splits-meminit-from-body.md](vptr-stamp-splits-meminit-from-body.md)
