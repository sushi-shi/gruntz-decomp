# A `goto` chain over DISTINCT bodies is a NESTED `if`, not a flat guard list
tags: cpp:goto cpp:branch cpp:if | asm:jcc asm:jmp | topic:codegen-idiom
symptoms: a probe/dispatch chain whose block skeleton matches retail for the first N blocks
and then permutes; one arm's body is emitted INLINE right after the probe that jumps to it,
with that probe's branch INVERTED, while retail emits the same body after the whole chain;
every later block shifts by one and the reported percent drops even though the instruction
multiset is unchanged
confidence: 9/10

cl 5.0 places a block with several predecessors immediately after its LAST predecessor and
inverts that predecessor's `jcc` to reach it by fall-through. A flat guard list

```cpp
eq = (strcmp(name, "A") == 0); if (eq) goto idle;
eq = (strcmp(name, "K") == 0); if (eq) goto idle;   // <- LAST predecessor of `idle`
eq = (strcmp(name, "E") == 0); if (eq) { ...E... goto store; }
...
idle:  /* body */
```

therefore emits `idle` right after the "K" probe (with `je nextProbe` instead of
`jne idle`), and drags the rest of the tail with it. Retail keeps `idle` where the
source put it. The fix is NOT a positive gate — the label is a real shared target, not a
redundant continuation (that case is `goto-continuation-label-is-not-a-shared-exit.md`).
Write the chain as NESTED `if`s so the shared body is simply the code that FOLLOWS the
nest, which makes the nest's exit its own last predecessor:

```cpp
if (ne) {                                       // the D probe
    eq = (strcmp(name, "A") == 0);
    if (!eq) {
        eq = (strcmp(name, "K") == 0);
        if (!eq) {
            ...E probe, I probe, M probe, codeI body, return...
        }
    }
    /* idle body */
    goto store;
}
walk: ...
store: ...
```

Both `jne idle` gates then survive un-inverted, because `idle` is the join of the nest and
no single probe owns it. Read the polarity straight off retail: `test cl,cl / jne <far>` at
BOTH probes means both are `if (!eq) { ... }` block openers, not `if (eq) goto`.

Companion in the same function: three loads and three stores through ONE base register
(`lea eax,[esi+0x43c]` then `[eax]`, `[eax+4]`, `[eax+8]`) is the implicit memberwise
`operator=`, i.e. `m_entranceCell = rec;` — field-by-field assignment splits the base
register per store.

Measured on `CGrunt::PlaySound` @0x4ac10: **66.95% -> 93.37%**, skeleton 70/70 blocks with
every edge `==` (it was 46 `!!` rows before). The struct assignment is worth the last 0.05.
