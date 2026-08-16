# The inline-expansion boundary is a general scheduling lever, not a ctor-only one - and it belongs in the .cpp, not in a shared header

tags: cpp:inline cpp:member cpp:ctor cpp:dtor | asm:mov asm:pop | topic:codegen-idiom topic:scheduling
symptoms: a function at 97-99.6% whose whole residue is ONE adjacent transposition -
a load hoisted over a store, a `mov ecx,<this>` receiver setup hoisted over the
preceding statement's stores, or an epilogue `pop` landing one slot early; instruction
multisets, CFG and relocation sets all identical
confidence: 9/10 (4 functions to EXACT, 2 negative controls, one attribution A/B)
variants: ctor-body-first-statement-is-an-inline-member.md, declaration-count-window-steers-regalloc.md

## The generalization

[`ctor-body-first-statement-is-an-inline-member.md`](ctor-body-first-statement-is-an-inline-member.md)
established the mechanism for one shape: cl 5.0's list scheduler hoists a statement's
memory access over a *successor-less* neighbouring instruction, and an inline-expansion
boundary takes that access out of the candidate set. Nothing in the mechanism is about
constructors or about vptr stores. Measured 2026-08-16, four functions, all `walls
diagnose` REGALLOC/SCHEDULING with identical multisets:

| function | residue before | spelling that closed it | after |
|---|---|---|---|
| `CSymParser::Clear` 0x13b850 | `mov esi,[edi+0x14]` (loop-head `m_list.m_head`) hoisted over `mov [edi+0x20],ebp` (`m_activeNode = NULL`) | loop head reads `HeadRezNode(m_list)` | 99.5493 -> **100.000** |
| `CSymRec::~CSymRec` 0x139cf0 | `mov ecx,ebx` (the member dtor's receiver) hoisted over BOTH trailing body stores; retail puts it BETWEEN them | second store written `SetSymRec(m_symNode, NULL)` | 98.3099 -> **100.000** |
| `CSymTab::CSymTab` 0x139de0 | `pop esi` one slot early, before the `m_parent` store instead of after | LAST body statement written `SetSymTab(m_node20, this)` | 97.3333 -> **100.000** |
| `CreateDoNothingNormal` 0xa9e00 | inlined `new CDoNothingNormal(owner)`: `mov eax,[esi+0x38]` hoisted over the leaf vptr stamp | ctor body `SetObjectFlags(1)` instead of `m_wwdObject->m_flags \|= 1` | 99.6129 -> **100.000** |

So the rule is: **the boundary pins whichever neighbour was free to float** - a receiver
setup, a spilled-register restore, an epilogue pop, a vptr stamp. Wrap the statement that
should come SECOND and the free instruction lands just before it instead of earlier.

## Put the boundary in the .cpp - a shared-header member costs 8 fresh regressions

The first spelling of `CSymParser::Clear`'s lever was a new member `CObjList::GetHead()`
in `include/Rez/RezList.h`. It closed the function, and it also moved every one of the 43
TUs that include that header: `verify check` went from clean to **8 fresh regressions**
(`CPlay::StepScroll` 100 -> 88.03, `CNetSession::Verify` 100 -> 89.53, four
`CDDrawShadeBlit` rows, `CPlay::LoadScrollSpeedOptions`, `CNetSession::Checksum`) - the
declaration-count window of
[`declaration-count-window-steers-regalloc.md`](declaration-count-window-steers-regalloc.md),
and a class-with-an-inline-member is the +11 handle stride of
[`tu-state-probe-family-decides-reachability.md`](tu-state-probe-family-decides-reachability.md).

Replacing it with a **file-local `static inline`** in the one `.cpp`

```cpp
static inline CRezItmBase* HeadRezNode(CObjList& list) {
    return list.m_head;
}
```

gave the SAME 100.000 and **zero** fresh regressions. Attribution control: none of
`Play.cpp`, `NetSessionMgr.cpp`, `DDrawShadeBlit.cpp` includes `DoNothingNormal.h`, all
three include `Rez/RezList.h` transitively, and the other two edits in that build were
`.cpp`-local.

What matters is only that the access sits inside an expansion. A free function taking the
containing object by reference works exactly like a member; `SymTab.cpp` already carried
`HeadSlotNode(DSoundList&)` and `PeekI32(const char*)` in that style before this work.
Prefer the narrowest home that still expands: `.cpp`-local first, the owner's header only
when several TUs genuinely need it (that is the CWapX case, which has 40-75 sites each).

## Negative controls - it is per-site, and it can cost points

* `CSymRec::CSymRec(i32,CSymTab*,i32,i32)` 0x139bf0: routing its FIRST body statement
  through `SetSymRec` took it 99.3548 -> **95.6452**. Its residue is an eax/edx pair
  rotation, not a transposition; the boundary changed the allocation order and lost.
  Written out again.
* `CAniPlayer::TickToggle` 0xe5b90: byte-identical either way (92.0000), so the boundary
  is not a universal improvement even inside a file where it closed two neighbours.

Read a drop after conversion as either "this residue was not a transposition" or the
`/Ob1` budget flip of the ctor pattern's `CProjectile` case, and write the site out again.

## Detection signature

`gruntz walls diagnose <rva>` says REGALLOC/SCHEDULING with equal byte length, instruction,
call, branch, `ret` and relocation counts, and an address-aligned base/target dump shows
exactly one adjacent pair swapped. If the pair is (statement access, free instruction) the
boundary is worth one A/B. If the pair is two *independent* loads/stores of the same kind -
two spill reloads at a join, the two halves of a u32->double conversion - it is the C1
handle-state coin of `tu-state-probe-family-decides-reachability.md` instead, and no source
spelling reaches it.
