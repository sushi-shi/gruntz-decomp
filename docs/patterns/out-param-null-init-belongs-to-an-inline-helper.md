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

## Screening: the codegen, not the source

A `T* x = NULL;` beside a `Lookup` is NOT itself the defect - cl schedules the
store after the pushes on its own at most sites, and converting one of those is
inert or harmful (a whole TU of 26 candidate sites yielded exactly one). Screen by
disassembling both sides of `build/objdiff/compare-new/{base,target}` and looking
at where the zero-store lands relative to that call's pushes. Two shortcuts:

* base `sub esp,N` LARGER than the target's is a strong positive - the caller-owned
  local takes its own slot where the helper's is homed in a dead parameter slot
  (`CWarpStoneFly::Init` 0x18 vs 0x14). It is not necessary, though: `PruneOrphans`
  had equal frames and was still the pattern.
* a frame delta with the stores already aligned means some OTHER local is extra;
  `CGiantRockLogic::BuildRockBreakInGameText` has the 0x18/0x14 delta and its
  lookup site is byte-identical.

## Evidence

| function | before | after |
|---|---|---|
| `CGameObject::AddLogicHit/Attack/Bump` 0x150f50/0x151030/0x151110 | 89.47 | **100.00** |
| `CWwdGameObjectA::ApplyLookupSprite` 0x1504d0 | 94.29 | **100.00** |
| `CGrunt::BuildEntranceAnimation` 0x67bd0 (5 sites) | 83.82 | **100.00** |
| `CGrunt::LoadWingzGruntSprites` 0x68880 (8 sites) | 89.80 | 93.75 |
| `CWarlord::CWarlord` 0x42d40 (11 sites, was a block macro) | 78.84 | 93.08 |
| `CWarpStoneFly::Init` 0x109bd0 | 91.33 | 96.62 |
| `CDDrawChildGroup::PruneOrphans` 0x15b1d0 | 93.75 | **100.00** |

`ApplyLookupSprite` had carried an `@early-stop` reading "96 mixed TU states and
35 local variants were byte-identical at this remaining slot" - none of those
variants moved the local's OWNERSHIP, which is the only lever that reaches it.

A block MACRO that declares the local is the caller's scope for this purpose:
`WARLORD_ANIM_LOOKUP(dst, suffix)` expanded `CAniElement* h = NULL; MapLookup(...);
dst = h;` eleven times in `CWarlord`'s ctor and every zero-store was hoisted ahead
of the `CString` concatenation calls that build the key. Only a real inline
function moves them.

## The miss path must ASSIGN, not `return`

When the helper has to reproduce retail's `Lookup(...) == 0 || out == NULL` test,
the spelling of its failure path decides between a branch and a mask:

```cpp
// cl folds the two exits into a select: `mov edx,[out]; neg eax; sbb eax,eax; and eax,edx`
if (!MapLookup(map, key, found)) { return NULL; }
return found;

// retail: `test eax,eax; je +4; mov eax,[out]` - the taken branch skips only the
// load, reusing the already-zero EAX as the result
if (MapLookup(map, key, found) == 0) { found = NULL; }
return found;
```

Both spellings are semantically identical and both sink the zero-store correctly;
`PruneOrphans` scored 91.72 with the early return and 100.00 with the assignment.
Swapping the arms (`if (MapLookup(...)) return found; return NULL;`) is the early
return again and scores the same 91.72.

## Cost: the helper is a declaration, and declarations are TU state

Each helper is one more named declaration ahead of everything below it. In
`wwdgameobject` a SECOND helper (for the `CMapStringToPtr`/`LeafCue` site in
`LookupAnimSprite`) flipped `CDDrawWorker::GetMemoryUsage`'s commutative
`imul` operand order (100.00 -> 99.96) three functions later; one helper did
not. Folding the two helpers into a template did not help - the instantiations
count too. Source spelling of the multiply (`h*w`, `w*h`, `h; h*=w`, a local for
`h`) is inert, so the flip is TU state, not the expression. Budget the helpers:
add the one that buys the most call sites.

## The helper's PARAMETER decides where the receiver chain is evaluated

Two helper shapes sink the zero-store equally well, and they are NOT
interchangeable - they differ in when the receiver is computed:

```cpp
// (a) map by reference: the caller evaluates the whole chain as the argument
static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name);
LookupWorker(m_host->m_imageRegistry->m_workersByName, key);

// (b) owner pointer: the chain is evaluated INSIDE the expansion
static inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* host, LPCTSTR name);
LookupWorker(m_page, key);
```

Shape (a) is right when the receiver needs more than one load, because cl splits
it: the first member load schedules early and `mov ecx,[r+N]; add ecx,K` stays
next to the call (`CSBI_MenuItem::ResolveFrame` 0xe81e0 92.45 -> **100.00**,
`CSpriteRefTable::Add` 0xe2890 97.50 -> **100.00**). Shape (b) is right when the
owner is ALREADY live in a register - a parameter, or a member the guard just
tested - because (a) then hoists the finished map address to the top of the block
and clobbers the owner: `CChatBox::ConfigureLeftCursorAnimation`/`Right` 0x182df0
/0x182e60 both went 94.87 -> **100.00** under (b) and were WORSE under (a).
Read which one retail used straight off the disassembly: `lea r,[&out]` before
the receiver load means (b), after it means (a).

This refines the `LoadCursorAnimation` counter-example in
[out-param-reset-between-arg-setup-and-call-is-in-the-helper.md](out-param-reset-between-arg-setup-and-call-is-in-the-helper.md):
`CPlay::LoadCursorAnimation` 0xd0920 regresses under (a) and is 97.62 -> **100.00**
under (b) (`LookupWorker(m_world, key)`), so "no literal push to schedule behind"
was the wrong reading - the receiver shape was.

More sites closed with the same two shapes: `CDDrawWorkerMapSmall::RemoveByKey`
0x165d30 77.66 -> **100.00**, `CDDrawWorkerB::Helper` 0x166040 95.00 -> **100.00**,
`CWwdGameObject::CreateNamed` 0x166780 94.12 -> **100.00**,
`CSpriteRefTable::LoadGruntzPalette` 0xe2d10 96.92 -> **100.00**,
`CDDrawWorkerHost::Read` 0x161640 96.72 -> 97.36,
`CSBI_SideTab::BuildHandle` 0xe9850 86.01 -> 90.04.

## When the helper costs a sibling, convert the TU's OTHER sites too

The declaration cost above is real but it is not a reason to stop at one helper.
In `sbi_menuitem` a single helper took `ResolveFrame` to EXACT and dropped the
unedited `CSBI_MenuItem::Render` 100.00 -> 74.04 (a fresh MAX-gate regression, so
uncommittable). Render is completely source-inert - four rewrites of its body and
a 0..20 typedef sweep are byte-identical - so the only variable was how many
declarations precede it. Converting the TU's two OTHER lookup sites (a
`LookupWorker(CDDrawSurfaceMgr*, LPCTSTR)` overload for `SerializeFields`, a
`LookupCue` for `SetState`) put THREE declarations ahead of Render and it returned
to **100.00** byte-for-byte, with `SetState` and `SerializeFields` still exact:
unit 98.36 -> 99.16, 9/11 -> 10/11. Two declarations alone were not enough (Render
recovered retail's 0x45 size but not its first four instructions). So: when the
sibling drop appears, finish the TU's conversion before judging the lever - each
site you were going to convert anyway is also a step of the window.
