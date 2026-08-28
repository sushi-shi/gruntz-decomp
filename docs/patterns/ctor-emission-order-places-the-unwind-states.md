# MSVC 5.0 emits a ctor as bases -> members -> vptr -> body, and the unwind states index that order

tags: cpp:ctor cpp:eh cpp:member | asm:mov | topic:codegen-idiom topic:eh
symptoms: `eh_frame --states` reports STATE_FLOW on a constructor; the two
sides store a different NUMBER of unwind states, or the same count at different points; a group
of member stores sits on the wrong side of the `mov [this],<vtable>` stamp
confidence: 9/10

## The order

cl 5.0 lays a constructor out in exactly four phases, and it never interleaves them:

1. base-class constructors, in declaration order;
2. member constructors, in declaration order;
3. the `mov DWORD PTR [this], OFFSET ??_7Class@@6B@` vptr stamp (**after** the members, not
   before them);
4. the constructor body.

Stores cannot be scheduled across a `call`, so any call in phase 1/2 pins everything around it.
That makes the boundary directly readable in the disassembly: **whatever is written before the
vptr stamp came from a mem-init, whatever is written after it came from the body.**

## The unwind states index phase 1+2

With `/GX` on, cl assigns one unwind state per destructible sub-object and stores the current
index before every point that can throw. So the state count is a *census of destructible bases
and members that were constructed by a mem-init*, and the sieve's ±1 rows are usually a
mem-init that has been written as a body assignment (or vice versa).

Three shapes, all measured 2026-08-08:

| retail | our source | fix |
|---|---|---|
| state 0 at the member's `call`, ours stores 2 | `m_hash.Construct(1);` in the body | `: m_hash(1)` mem-init (`CRezArchive` 76.68 -> **100.00**) |
| first/last zeroed BEFORE the vptr stamp | assigning the links in the containing ctor body | the ctor belongs to the member's real `CVirtBaseList` base, whose own ctor initializes `m_pFirst`/`m_pLast` |
| an extra state, then three field stores | `m_gameObject = owner; ...` in the body | the fields belong to a real base - `: CMovingLogic(owner), CWapX(owner)` (`CProjectile` 86.23 -> **99.78**) |

## The same boundary is a field-OWNERSHIP oracle, with or without /GX states

Phase 2 is the *base's* mem-init as well as the derived class's, so a scalar
written BEFORE the vptr stamp that your model puts on the DERIVED class is
evidence the field belongs to a BASE. This reads off the store order alone - no
unwind states, no destructible members needed.

`SoundVolumeRamp::SoundVolumeRamp` 0x136fe0 writes `+0xc, +0x10, +0x14` in one
uninterrupted run and only then stamps `??_7SoundVolumeRamp@@6B@`. Modelling the
first two as base fields (`SoundTask`) and leaving `m_stopAndRewind` at
`+0x14` on the derived class makes it a body statement, and no body order can
put it back in that run: all six orders of the remaining three assignments
plateau at 85.18, and the one order that does reproduce retail's *store* order
still leaves the value's load three slots late in the schedule. Giving the base
the third field and a third ctor argument closed the function 88.00 ->
**100.00 EXACT**, with the body then in plain declaration order.

Corroborate before moving a field: the empty dtor the ctor's unwind funclet
jumps to stamps the vptr of the class it destroys (`mov [ecx],??_7Base@@6B@`),
which names the base directly, and the module's vtable set says how many classes
are in the chain at all.

A fourth state-flow shape has no member at all: **an extra state with a `call` right after it, at the end of
a `new`-expression's protected region, is the constructor BODY.** `new CMenuTree` stores state 3
after the last CString and then calls `InitializeMembers()`; that only happens if
`CMenuTree::CMenuTree()` calls `InitializeMembers()` itself. Writing
`p = new CMenuTree; p->InitializeMembers();` puts the call outside the region and
state 3 disappears (`CMenuState::LoadGameAssetNamespaces` 89.39 -> 95.14).

## How to read a row

1. Find the state slot: it is the `push -1` slot, i.e. `[esp + frame]` where the epilogue's
   `mov ecx,[esp+N]` reads the saved `fs:[0]` four bytes below it. Track the `push`es - the same
   slot is spelled with different displacements at different depths.
2. Walk retail's stores in order and mark the vptr stamp. Everything before it is phase 1/2.
3. Count the calls between state stores. A state store with no call before the next one is a
   member whose ctor inlined to nothing but which still has a destructor.

## State-flow shapes that are not another object

Not every ±1 row is an object.

* **An out-parameter that lands in the state slot's neighbourhood.** `CLightFx::CLightFx` reads
  as +1 because retail's inlined `CMapStringToOb::Lookup` zeroes its `CObject*` out-parameter in
  the *incoming argument's* home slot, which the slot heuristic scores as a state store. The real
  divergence there is an inline cut, not an object.
* **A duplicated epilogue.** Each copy of a destructor call carries its own state store, so a
function whose early `return` cl duplicated instead of tail-merging reads as +1
(`CFontConfig::RenderInputText`). That is `exit_merge_sieve`'s lever.
* **A duplicated ordinary-call region while one object remains live.** The same
  state value is stored again without another ctor/dtor. `CGrunt::StepArrivalDrop`
  has retail 8 versus base 7 stores but both sides use only `{-1,0,1}` and own the
  same single `CPtrList`; retail also has one extra `RemoveHead` flow site.

related: [eh-frame-presence-is-a-source-fact.md](eh-frame-presence-is-a-source-fact.md),
[goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md),
[ctor-inline-cut-depth-varies-per-new-site.md](ctor-inline-cut-depth-varies-per-new-site.md)
