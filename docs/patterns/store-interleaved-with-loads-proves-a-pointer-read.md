# Retail INTERLEAVES a store with the next loads: the fields are read through a pointer

**Tags:** `cpp:pointer` `cpp:struct` `cpp:member` | `asm:mov` `asm:sub` | `topic:codegen-idiom`

**Symptom.** Two statements read four fields of one object and write two others.
Retail emits `load / load / sub / STORE / load / load / sub / store`; ours emits
`load / load / load / load / sub / sub / store / store`. Same instructions, same
count, so a masked diff calls it scheduling. `walls reloadscan` names it as a
displacement one side re-reads across a call and the other never does.

## The rule

cl 5.0 has no type-based alias analysis. A read written as `obj->field` is known to
be disjoint from a store written as `obj->other`, so cl freely hoists every such load
above every such store. A read written through a POINTER — `p->field`, where `p` is a
`T*` local — may alias any store cl cannot prove disjoint, so **cl is forbidden to
move it past the store** and the two statements stay in source order.

So retail declining the hoist is the evidence:

```cpp
// cl hoists all four loads above both stores - the object base folds in, and
// three of the four reads come out `[grid+0x64]`-style even when a pointer to
// the member exists in a register for an earlier call
grid->m_gridW = grid->m_bounds.right  - grid->m_bounds.left;
grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;

// cl cannot hoist rd->bottom past the m_gridW store; every field is read
// through rd, and the store lands between the two subtractions
RECT* rd = &grid->m_bounds;
grid->m_gridW = rd->right  - rd->left;
grid->m_gridH = rd->bottom - rd->top;
```

Retail, `CGrunt::PathScan` 0x57db0, every `SCAN_BOUNDS` site:

```
mov edx,[edi+0x8] / mov ebx,[edi] / sub edx,ebx
mov [esi+0x70],edx                  <- m_gridW stored between the two pairs
mov eax,[edi+0xc] / mov ecx,[edi+0x4] / sub eax,ecx
```

`edi` is `&grid->m_bounds`, already materialized for the `IntersectRect` call above.
Retail reaches ALL FOUR fields through it; our object-base spelling folded three of
them back onto `grid` and hoisted the lot. 89.19 -> 90.58, 725 instructions -> 723
against retail's 723, and the sieve's three exclusives vanish.

## Reverse use

The reading runs both ways, and the direction matters:

* **Retail interleaves, we hoist** -> retail's source binds a pointer. Bind it too.
  Prefer the pointer already in a register for a neighbouring by-address call, which
  is usually why the dev named it in the first place.
* **We interleave, retail hoists** -> our pointer is invented. Read the member off
  the object.

Do not read a *uniform* `[reg+N]` base as evidence on its own: cl folds
`&obj->member` into the object base whenever it can, so a pointer spelling can still
produce object-relative addressing. The load/store ORDER is the signal, and it is the
part a masked diff cancels.

## Family

Same frame, different optimization: `escaped-object-blocks-licm-not-the-copy.md`
(hoisting out of a LOOP declined because the object's address escaped) and
`post-call-inline-helper-needs-call-stable-inputs.md` (a reload after an opaque call).
`gruntz walls reloadscan` sieves all three - the `--call` channel is the one that
fires here.
