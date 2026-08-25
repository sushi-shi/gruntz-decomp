# Pinning a base constructor out-of-line to claim one RVA costs every derived ctor ~45 points

- **confidence** c10
- **tags** `cpp:ctor` `cpp:inline` `cpp:class` | `asm:call` `asm:mov` | `topic:codegen-idiom`

## Symptom

A whole FAMILY of leaf constructors sits at 40-55% with the same diff shape: the base's
prologue is short (`push ecx` / `sub esp,0x10`) where retail's is long (`sub esp,0x14` +
three saves), and the base has a single `call <tgt>` where retail has 60-100 instructions
of member stores, two vptr stamps and a guarded registration block.

```
 base                                   target (retail)
 mov  esi,ecx                           mov  esi,ecx
 push edi                               push edi
 mov  [esp+0xc],esi                     mov  [esp+0xc],esi
 call <ctor of the base class>          mov  DWORD PTR [esi],0x5e70b4
 mov  [esi+0x34],edi                    lea  ebp,[esi+0x18]
                                        ... 90 more inlined instructions ...
```

That single `call` IS the whole diff: retail EXPANDS the shared base constructor into every
leaf, so a base modelled with one out-of-line body loses the entire expansion in each of them.

## Cause

Someone moved the base ctor out of its shared header into a `.cpp` in order to bind its
retail RVA (`RVA(0x00058cd0, 0x195)`), because with the ctor inline no reconstructed caller
was big enough for cl to decline the inline, so no TU emitted the standalone COMDAT and the
retail row read as unmatched. **That trade is catastrophically negative.** It buys one
function and taxes every derived constructor in the tree.

Measured on `CUserLogic::CUserLogic(CGameObject*)` (2026-08-07), 65 derived ctors:

| | before (OOL in MotionState.cpp) | after (inline in UserLogic.h) |
|---|---|---|
| ctors >= 90% | 5 | 45 |
| ctors < 60% | 40 | 4 |
| Overall tree fuzzy | 86.19% | 86.97% |
| exact functions | 3291 | 3295 |

Individual moves: `CWarpStonePad` 43.8 -> 99.7, `CSingleAnimation` 44.6 -> 99.6,
`CTileTriggerSwitch` 48.3 -> 100.0 EXACT, `CActionArea` 40.8 -> 94.4, `CTeleporter`
45.6 -> 92.1, `CExplosion` 45.3 -> 92.3, `CToyPeek` 40.1 -> 89.3, `CProjectile`
34.3 -> 60.5. One function lost its binding: 0x58cd0 itself.

## How to tell which shape retail used

Count the CALLERS of the retail body with `gruntz sema xref <rva>`. If the caller list is a
handful of *specific* functions while dozens of same-family constructors do NOT appear, the
ctor was **inline in the header** and those few callers merely exhausted cl's inline budget.
`CUserLogic::CUserLogic` has exactly three: `CGrunt::CGrunt`, `CProjectile::CProjectile`,
`DispatchDoNothingNormalLogic` - all three still incomplete reconstructions here, which is precisely
why our cl inlines where retail called.

The inlined copy is not a byte-copy of the standalone body: cl inlines *deeper* in the
standalone (retail's 0x58cd0 has `BuildLogicTypeTable` expanded inside it) and stops at the
first level in the leaves (they emit `call <BuildLogicTypeTable>`). Seeing the helper called
in the leaf and expanded in the standalone is confirmation of the inline model, not a
contradiction of it.

## Rule

> **AMENDED (2026-08-08).** The either/or below is not forced. A tagged INLINE sibling
> (`CUserLogic(CGameObject*, ENoSeed)`) lets the sixty-five derived ctors keep the expansion
> while the three real retail callers take a pinned out-of-line body, so both the family AND
> the row are matched - see [two-shapes-need-two-entities.md](two-shapes-need-two-entities.md).
> The tag must sit on the INLINE sibling: on the pinned ctor it would change `ret 4` to `ret 8`.
> Until that rewrite lands (65 initialiser lists), the rule below is still the right default.

Model the base ctor **inline in its header**, and leave the standalone RVA unmatched until a
real caller grows big enough to emit it (`llvm-nm build/objdiff/base/*.obj | grep <mangled>`
tells you if any TU already does). Never introduce an emitter-only TU or an out-of-line
definition to farm the one row - see
[inline-base-ctor-emission-wall.md](inline-base-ctor-emission-wall.md) for the standing
doctrine and [no-ifdef-guard-devices] for why a per-TU `#ifdef` switch is not the answer
either. Losing the function's census row (`config/retail/gruntz_functions.tsv`,
deletion committed) is the accepted cost, and it is one row against sixty-five.

## Refinement (2026-08-08): pin the SMALL leaf ctors, keep the expanded one inline

The rule is about the ctor the family *expands*, not about every ctor in the chain. In a
chain `CLoadable -> CResolveNode -> CGameObject -> CWwdGameObjectA/C/F` retail expands
`CGameObject::CGameObject` (0x15b390) into every `Create*Object` new-site and, inside that
expansion, **calls** `CResolveNode::CResolveNode` (0x15b2c0), `CLogicRecord::CLogicRecord`
(0x15b300) and `CAniAdvanceCursor::CAniAdvanceCursor` (0x15b730) out-of-line. Three
configurations, measured whole-tree on the same worktree:

| | exact | fuzzy |
|---|---|---|
| all four inline in headers (the naive model) | 3323 | 89.12% |
| all four pinned out-of-line | 3323 | 89.14% |
| **three leaf ctors pinned, `CGameObject::CGameObject` inline** | **3326** | **89.17%** |

The winning shape moves `CDDrawChildGroup::CreateDeferredObject` 62.87 -> 93.39,
`CreateDotObject` 65.67 -> 94.40, `CreateSpriteObject` 66.73 -> 94.56 and banks three new
EXACT rows, at no denominator cost (`levelplane` -3 / `wwdfactoryobject` +3, a pure
transfer the RVA-keyed function census passes on its own). Pinning `CGameObject::CGameObject` as well costs
all three `Create*Object` rows ~45 points each - exactly the tax this pattern warns about.

So run the `sema xref` caller test on **each** ctor in the chain separately: the one whose
callers are the *family* stays inline; the ones the family's expansion *calls* get pinned.

### The leaf set is SIX, not three - and the sixth pin needs a tagged inline sibling

`0x15b270`-`0x15b340` is one contiguous run of leaf ctors and the `Create*Object` expansion
calls **all** of them. Three more sit beside the three above: `WwdDirtyRect::WwdDirtyRect`
(0x15b270), `WwdGridNode::WwdGridNode` (0x15b2a0) and `WwdRegion::WwdRegion` (0x15b2b0).
Pinning those three as well, on top of the config in the table, banks three more EXACT rows
and finishes the factories: `CreateSpriteObject` 94.56 -> **100.00 EXACT**, `CreateDotObject`
94.40 -> 96.89, `CreateDeferredObject` 93.39 -> 96.17 (whole tree 3407 -> 3416, 89.21% ->
89.27%).

They do not compose for free. `WwdDirtyRect` is a MEMBER of `CResolveNode`, and retail's
pinned `CResolveNode::CResolveNode` (0x15b2c0) is 61 bytes of straight-line stores ending
`ret 0xc` that seed it **inline** (`[+0x20]=COORD_UNSET`, `[+0x38]=-1`) and call nothing. So
once `WwdDirtyRect::WwdDirtyRect` is pinned, both `CResolveNode` ctors must construct
`m_dirty` through a tagged INLINE sibling:

```cpp
CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 a, i32 b)
    : CLoadable(owner, a, b), m_dirty(WwdDirtyRect::INLINE_SEED) { ... }
```

Measured A/B with every pin held constant and only the tag removed: `??0CResolveNode@@QAE@XZ`
and `??0CResolveNode@@QAE@PAVCDDrawSurfaceMgr@@HH@Z` both fall out of EXACT (3416 -> 3414),
and the untagged base obj grows retail's absent `push -1; push 0; mov eax,fs:[0]` EH frame
from the call it now has to make. The tag is not decoration: it is what lets the two pins
coexist. See [two-shapes-need-two-entities.md](two-shapes-need-two-entities.md).

related: [inline-base-ctor-emission-wall.md](inline-base-ctor-emission-wall.md),
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md)
(the frame delta this produces is the visible symptom),
[sortkey-flag-rmw-needs-local-receiver.md](sortkey-flag-rmw-needs-local-receiver.md)
(the residue left over once the expansion is right).

## RESOLVED (2026-08-08): the trade is not forced

The "one row vs sixty-five" trade only exists while the ctor is ONE entity. A tag parameter on
an inline sibling plus a shared inline body helper gives both shapes at once:
`??0CUserLogic@@QAE@PAUCGameObject@@@Z` (0x58cd0) is now 100.00 EXACT with every derived ctor
keeping its expansion. See
[nested-ctor-call-vs-expansion-is-a-tu-visibility-split.md](nested-ctor-call-vs-expansion-is-a-tu-visibility-split.md).
