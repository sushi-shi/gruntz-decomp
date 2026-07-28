# A `goto shared;` gate INLINES the shared block — invert it so the body is the fall-through

tags: cpp:branch cpp:goto cpp:loop | asm:je asm:jne | topic:codegen-idiom
symptoms: jcc_sieve POLARITY on the LAST gate before a big body, retail `je <far shared
tail>` where base has `jne <body>` with the shared tail sitting inline right after it,
ret counts equal
confidence: 8/10

When several early gates share one exit block, hoisting them all to `goto shared;`
is only half the recipe. cl places the shared block immediately after the **last**
`goto` that targets it, so that final gate becomes `jne <body>` with the latch inline —
retail instead has `je <far shared tail>` and falls straight into the body. Spell the
last gate as its complement, wrapping the body in the `if`, and let control fall out
of the `if` into the shared block:

```cpp
// before - the last gate is a goto; cl inlines `ready:` right there
if (count == 0) {
    goto ready;
}
SendStatFlag(0x3ed, 1);
… big body …
return 1;
ready:
m_534 = 1;
return 1;

// after - the body is the fall-through, `ready:` stays at the bottom
if (count != 0) {
    SendStatFlag(0x3ed, 1);
    … big body …
    return 1;
}
ready:
m_534 = 1;
return 1;
```

```asm
target: cmp edx,ebp | je  READY | push edi | push 0x3ed | …   ; body falls through
base:   cmp edx,ebp | jne BODY  | mov [esi+0x534],ebx | …     ; latch inlined here
```

STEERABLE. Earlier gates that jump to the same label stay `goto shared;` — only the
last one flips. Evidence (2026-07-28): `CMulti::WaitForOtherPlayers` @0x0bb700
80.47 → 91.66 on this one inversion (the function already used `goto ready;` for both
gates and was filed as three compounding codegen walls); a same-pass re-read of the
global instead of a spilled local and a `SIZE` copy of the mode pair took it to 94.42.
Same family as [dup-exit-means-a-shared-goto-label.md](dup-exit-means-a-shared-goto-label.md),
seen from the other side.
