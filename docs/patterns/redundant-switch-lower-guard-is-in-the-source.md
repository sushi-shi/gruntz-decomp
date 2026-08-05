# A redundant `cmp x,MIN; jl default` in front of a switch is an explicit source guard

tags: cpp:switch cpp:branch | asm:cmp asm:jl asm:ja | topic:codegen-idiom
symptoms: retail dispatches a dense switch with `cmp eax,MIN; jl <default>` and THEN the
  switch's own `add eax,-MIN; cmp eax,SPAN; ja <default>`; the recompile emits only the
  second pair, so it is two instructions short and everything after shifts
confidence: 9/10

The unsigned range check `add x,-MIN; cmp x,SPAN; ja` already rejects everything below
MIN (negatives wrap), so the signed lower-bound test in front of it is dead. cl5 does not
invent it — it comes from a guard the source actually wrote:

```cpp
if (cursor < CURSOR_TOOL_FIRST) {
    return 0;
}
switch (cursor) { case CURSOR_TOOL_HANDZ: … }
```

`CPlay::LoadCursorSprites` @0xd0120, 98.23 → 99.02.

**It is not the switch expression's type.** Switching between the enum and the raw `int`
was measured byte-identical (both emit only the unsigned pair), as was moving the guard
into the `default:` arm. The guard has to be a statement ahead of the switch.

When you add one, name the band edge it compares against — the `enum-domains` gate rejects
a range test against a plain MEMBER of the domain, so declare a `_FIRST`/`_BEGIN` marker at
that value and compare against it (rename only, byte-neutral).

related: switch-key-unsigned-ja-vs-jg.md, switch-density-byte-index-table-vs-tree.md,
enum-domains.md
