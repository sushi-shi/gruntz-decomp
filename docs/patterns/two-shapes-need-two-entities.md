# One helper with BOTH a `call` and an expansion is TWO source entities, not one inline

- **confidence** c10
- **tags** `cpp:inline` `cpp:call` `cpp:ctor` `cpp:class` | `asm:call` | `topic:codegen-idiom` `topic:wall`

## Symptom

A leaf helper appears in the retail image in two shapes at once: real `call` sites (often
through one ILT thunk) *and* open-coded expansions - sometimes both inside the SAME
function. Modelling it as one in-class inline reproduces every expansion but converts the
`call` sites into expansions too, and then **no base obj emits the COMDAT at all**, so the
retail RVA silently leaves scoring. Modelling it as one out-of-line body reproduces the
calls and loses every expansion.

## cl 5 cannot produce both shapes from one definition. Probed, not assumed.

| probe (`/nologo /c /O2 /MT /GX`) | expansions | COMDAT emitted? |
|---|---|---|
| in-class inline, 2 callers | yes | **no** |
| in-class inline, 5 callers | yes | **no** |
| in-class inline, **60** callers in one 14 KB function | yes | **no** |
| out-of-class `inline C::F()` in the same file | yes | **no** |
| in-class inline + `#pragma auto_inline(off)` | yes | **no** |
| **out-of-line definition in the same TU** | **no** - every site is a `call` | yes |
| in-class inline + `/Ob0` | no | yes |
| in-class inline + **address taken** | yes | yes |

There is no inline BUDGET to exhaust: 60 expansions in one caller and cl still never
declines. **NARROWED 2026-08-14** — that is a property of THESE callees, not of cl. A
callee with `cb <= 0x28` is budget-EXEMPT, so no caller size can decline it and the
60-site probe could never have rejected; `PointInRect` and `CDDrawWorkerCache::Find`
are both under it. Content that emits NOTHING (a release `ASSERT`, an unused local)
lifts `cb` over 0x28 with byte-identical output and the same callee then declines 8 of
30 — so before concluding a split was per-TU visibility, titrate `cb`. See
zero-emission-statements-cross-the-ob1-cb-exemption.md. `/Ob0` is per-TU and kills
every other inline in it. Address-taking works but
fabricates a global with a DIR32 reloc, and retail has **no** data reference to any of
these bodies (scanned every section for the VA - zero hits), so it is not what retail did.

So when the retail image shows both shapes, **the retail source had two entities.** Model
two.

## The recipe: an inline sibling plus a one-line out-of-line wrapper

```cpp
// GameLevel.h
class CGameLevel {
    // the ~84 expansion sites call THIS
    static i32 PointInRect(const LevelCoordRect* r, i32 x, i32 y) {
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) { return 1; }
        return 0;
    }
    // the 30 retail `call` sites call THIS
    static i32 PointInBounds(const LevelCoordRect* r, i32 x, i32 y);
};

// GameLevel.cpp
RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    return PointInRect(r, x, y);
}
```

The wrapper inlines its sibling, so the emitted COMDAT is byte-identical to retail's
standalone body - verified for `CGameLevel::PointInBounds` (0x6b330, 42 B) and
`CGameLevel::ResetParamBlock` (0x15d170, 115 B, including retail's `edx`/`eax` constant
CSE). **One textual copy of the logic**, so no clone signature appears.

Split the sites by evidence, never by guess. Scan `.text` for the two shapes and attribute
each to its containing function:

- a **call** is `... push <arg>; call <thunk>` - for a rect argument, `add reg,0x40; push reg; call`
- an **expansion** is the same `add reg,0x40` followed directly by the compare block

Then, inside a function that has one of each, discriminate by the neighbouring stores and
the callee that follows the test (e.g. `push 0x8; call <LoadGruntSpawnConfig>` vs
`push <cueId>; call <SpawnVoiceDriver>`). **Do not use RVA order** - /O2 reorders blocks, and
in `CGrunt::UpdateArrival` (0xf0130) the expansion precedes the call in the image while the
call precedes the expansion in the source.

## Constructors: put the tag on the INLINE sibling, never on the pinned body

A ctor cannot be overloaded by name, so the two entities are separated by a tag parameter -
and which one carries the tag is load-bearing:

```cpp
struct WwdGridNode : DSoundLink {
    WwdGridNode();                        // OUT-OF-LINE, pinned at 0x15b2a0
    enum ENoSeed { NO_SEED };
    WwdGridNode(ENoSeed) {}               // inline sibling: the tag lives HERE
};

// WwdFactoryObject.cpp - retail inlines the base's two stores into this body
RVA(0x0015b2b0, 0xe)
WwdRegion::WwdRegion() : WwdGridNode(WwdGridNode::NO_SEED) {
    m_bucket = NULL;
    m_reserved08 = 0;
    m_object = NULL;
}
```

A tag parameter on the *pinned* ctor would add a pushed argument and turn `ret 4` into
`ret 8`, so it can never byte-match. The tagged sibling is always inlined, so its extra
parameter costs nothing.

## What this supersedes

`inline-visibility-splits-call-and-expansion.md` concluded that retail's `call` sites must
have seen a declaration-only header and that "there is no single-body C++ spelling that
yields both shapes". The first half is unprovable and the second is beside the point: there
is no single-BODY spelling, but there is a two-ENTITY one, and it needs no per-TU include
split and no `#ifdef` device.

## The tag split is what lets two neighbouring pins coexist

`base-ctor-pinned-out-of-line-costs-every-derived-ctor.md`'s refinement pins the leaf ctors
a family's expansion CALLS. When two of those leaves nest - `WwdDirtyRect` is a member of
`CResolveNode`, and both have their own retail COMDAT - pinning the inner one breaks the
outer one, because retail's outer body seeds the inner member INLINE. A/B with every pin
held constant and only the tag removed: 3416 -> 3414 exact, and the untagged base obj grows
an EH frame retail does not have. So the two campaigns' answers are not alternatives; the
tag split is the joint that makes the second pin safe.

`base-ctor-pinned-out-of-line-costs-every-derived-ctor.md` measured pinning
`CUserLogic::CUserLogic` out-of-line as costing 65 derived ctors ~45 points each, and ruled
"never introduce an out-of-line definition to farm the one row". With the tag split that
trade disappears: the 65 derived ctors take the tagged inline sibling and the three retail
`call` sites (`CGrunt::CGrunt`, `CProjectile::CProjectile`, `CreateDoNothingNormal`) take
the pinned body. That rewrite is not done yet - it needs the tag threaded through all 65
derived initialiser lists - but the wall it documents is not a wall.

## Why a lost COMDAT is worse than a regression

`0x6b330` and `0x15d170` both carried MAX 100.0000 and were emitted by **no** base obj at
all. Nothing scored them, so the fuzzy % barely moved and the loss read as noise. Check for
it directly:

    llvm-nm build/objdiff/base/*.obj | grep '<mangled>'

Six rows in `config/match_baseline.tsv` were in this state (all recovered to 100.00):
`?PointInBounds@CGameLevel@@SAHPBUtagRECT@@HH@Z`, `?ResetParamBlock@CGameLevel@@QAEXXZ`,
`??0WwdDirtyRect@@QAE@XZ`, `??0WwdGridNode@@QAE@XZ`, `??0WwdRegion@@QAE@XZ`,
`??1CWorldSoundSet@@QAE@XZ`. Two remain: `??0CGameObject@@QAE@PAVCDDrawSurfaceMgr@@HH@Z`
(0x15b390) and `??0CUserLogic@@QAE@PAUCGameObject@@@Z` (0x58cd0).

related: [inline-visibility-splits-call-and-expansion.md](inline-visibility-splits-call-and-expansion.md),
[base-ctor-pinned-out-of-line-costs-every-derived-ctor.md](base-ctor-pinned-out-of-line-costs-every-derived-ctor.md),
[inline-base-ctor-emission-wall.md](inline-base-ctor-emission-wall.md),
[shared-inline-transcribed-once-per-call-site.md](shared-inline-transcribed-once-per-call-site.md)
