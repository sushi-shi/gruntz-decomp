# An out-param's `= NULL` sinks below the argument pushes when an inline helper owns the local

tags: cpp:local cpp:inline cpp:decl | asm:push asm:lea asm:mov | topic:regalloc topic:codegen-idiom
symptoms: `walls diagnose` says REGALLOC/SCHEDULING with equal calls/branches;
ours stores the zero into the out-local BEFORE (or between) the callee's
argument pushes, retail stores it AFTER the last push - so the store's `[esp+N]`
displacement differs by 4 (or 8) on every such call site, and the frame can be
one slot larger (`sub esp,0x10` vs `sub esp,0xc`)
confidence: 9/10

## The mechanism

A by-reference lookup is written in the caller as

```cpp
CObject* out = 0;
map.Lookup(key, out);
use(static_cast<T*>(out));
```

The zero-store, the `lea` of `&out` and the argument pushes are all in one basic
block, so their order is a scheduling decision. With the local declared in the
CALLER's scope the store is scheduled with the caller's other IL - typically
first, before the pushes it "belongs" to. Retail schedules it last, after both
pushes.

Wrapping the same statements in an inline helper that OWNS the local moves the
zero-store into the inlined body's own IL, and cl schedules it with the call it
belongs to - after the pushes, exactly as retail:

```cpp
static inline T* LookupObj(CMapStringToOb& map, LPCTSTR name) {
    CObject* result = NULL;
    map.Lookup(name, result);
    return static_cast<T*>(result);
}
...
EnsureHitWorker(LookupObj(OwnerMgr()->m_workerCache->m_workers, key));
```

Retail's own source clearly used such helpers: in `WwdGameObject.cpp` the two
functions that were already EXACT (`ApplyName`, `ApplyLookupGeometry`) are the
two that call a helper, and the four below 100 were the ones with the local
inline. That asymmetry inside one TU is the detection signature.

## Evidence

| function | before | after |
|---|---|---|
| `CGameObject::AddLogicHit/Attack/Bump` 0x150f50/0x151030/0x151110 | 89.47 | **100.00** |
| `CWwdGameObjectA::ApplyLookupSprite` 0x1504d0 | 94.29 | **100.00** |
| `CGrunt::BuildEntranceAnimation` 0x67bd0 (5 sites) | 83.82 | **100.00** |
| `CGrunt::LoadWingzGruntSprites` 0x68880 (8 sites) | 89.80 | 93.75 |

`ApplyLookupSprite` had carried an `@early-stop` reading "96 mixed TU states and
35 local variants were byte-identical at this remaining slot" - none of those
variants moved the local's OWNERSHIP, which is the only lever that reaches it.

## Cost: the helper is a declaration, and declarations are TU state

Each helper is one more named declaration ahead of everything below it. In
`wwdgameobject` a SECOND helper (for the `CMapStringToPtr`/`LeafCue` site in
`LookupAnimSprite`) flipped `CDDrawWorker::GetMemoryUsage`'s commutative
`imul` operand order (100.00 -> 99.96) three functions later; one helper did
not. Folding the two helpers into a template did not help - the instantiations
count too. Source spelling of the multiply (`h*w`, `w*h`, `h; h*=w`, a local for
`h`) is inert, so the flip is TU state, not the expression. Budget the helpers:
add the one that buys the most call sites.
