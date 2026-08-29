# An inline signed selector can recover a caller's allocation island

tags: cpp:inline cpp:local cpp:ternary cpp:return | asm:neg asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: a large function has the right calls and loop behavior but the wrong
return value, too many source returns, and a callee-saved-register rotation;
restoring the shared result fixes the CFG, then extracting a tiny sign selector
changes the caller's allocation even though the helper leaves no call
confidence: 9/10

`CGameLevel::MoveToward` exposes two source layers that must be restored in
order. Retail's final slow-path return does not synthesize a Boolean: it returns
the last `DispatchMove` result in EAX. The three fast paths return that same
callee result. A function-scope result assigned in an `if`/`else if` tree and
returned once models that contract; returning the loop-control flag does not.

The slow setup then selects a signed magnitude twice. Writing the comparisons
in the caller left cl 5.0 on the lower allocation island. A file-local inline
selector supplies a distinct C1 tuple boundary and recovers the retail-sized
caller:

```cpp
static inline i32 SignedStepToward(i32 current, i32 goal, i32 magnitude) {
    return current > goal ? -magnitude : magnitude;
}
```

The controlled progression at RVA `0x15de40` was:

- parameter reuse with the wrong return/control layer: 78.43%;
- shared `flags` result and one source tail: 82.71%;
- the inline ternary sign selector composed on that base: 87.2168%, exact
  0x164-byte extent, 145/145 decoded instructions, 4/4 calls, 21/21 branches,
  4/4 returns, 4/4 relocations, and equal 8-byte frames.

Sixteen selector bodies produced four compiler islands. The ternary and its
early-return twin were best; mutation/assignment and reference-output forms
were lower. A 75-cell local-lifetime matrix, 216 helper-parameter-order cells,
96 target-adjacent C1 trials, and a 512-shape depth-1..3 AST campaign found no
higher state. The remaining residue is an EBP/EDI role exchange plus the
resulting memory-operand choices.

The clamp layer is separately bounded. Keeping `StepTowardGoal` and
`IsWithinStep` as inline helpers is byte-flat against their direct expansions,
so the INLINE/MACRO PRIOR keeps the abstraction. Adding nested inline
`Minimum`/`Maximum` helpers creates a lower 86.4126% island; composing 96 C1
states and another 512 source shapes on that lower base is completely flat.
Contemporary sibling code proves that inline/macro clamp families existed, but
does not prove that Gruntz used that additional nested layer, so it is not kept.

Reverse-use rule: when retail returns the last callee result, first restore a
shared result variable and source-level tail. If a repeated sign-selection
fragment remains inside a larger allocation wall, test it as a real inline
helper before permuting its arithmetic. Preserve the helper when direct
expansion is byte-flat; do not infer an extra generic clamp layer from the
clamping semantics alone.

related: [inline-helper-supplies-the-il-tuple-a-colour-row-needs](inline-helper-supplies-the-il-tuple-a-colour-row-needs.md), [int-return-that-retail-never-sets-is-a-void](int-return-that-retail-never-sets-is-a-void.md), [guard-lifetime-enables-postincrement-pair](guard-lifetime-enables-postincrement-pair.md)
