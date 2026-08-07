# `mov [this+N],<zero reg>` where retail RE-READS the member it just tested against NULL

tags: cpp:if cpp:member cpp:assign | asm:cmp asm:mov | topic:wall topic:regalloc
symptoms: `if (p->f == NULL) { m_x = p->f; ... }` — ours stores the function's zero register into `m_x`, retail emits an extra `mov eax,[p+N] / mov [this+M],eax`; the function is exactly ONE instruction short and, if the extra zero use tips the allocator, an entire callee-saved register (`push ebx / xor ebx,ebx`) appears that retail does not have
confidence: 8/10

After `cmp [p+N],<zeroreg> / jne`, cl5 in our build records the equality and
substitutes the register at the guarded read, collapsing `m_x = p->f;` to a
register store. Retail's compiland does not — it re-loads `[p+N]` inside the
branch, even though it also has a zero in a register at the compare. Same
compiler, so the propagation must be source-sensitive, but nothing found so far
moves it:

* a local for the tested value (`CAniElement* cur = p->f; if (cur == NULL)`);
* a local for the RECEIVER (`CWwdGameObjectA* w = m_wwdObject;` used for all three uses);
* `!p->f` instead of `p->f == NULL`;
* re-spelling the guarded read through the OTHER member that holds the same
  pointer (`m_object` vs `m_wwdObject`) — this DOES defeat the propagation but
  costs an extra pointer load, so the count overshoots by one;
* the sibling `CEyeCandy` form of the same thing (`if (o->m_sortKey == 0 && ...)`
  then `if (o->m_sortKey != v)`) resists an inline helper taking the receiver by
  pointer, and re-spelling the guard on `m_object` defeats the propagation but
  reintroduces the memory-form OR
  ([sortkey-flag-rmw-needs-local-receiver.md](sortkey-flag-rmw-needs-local-receiver.md)).

The one-instruction shortfall is cheap on its own, but it can be expensive
indirectly: the substituted store is an EXTRA use of the constant 0, and in
`CFrontCandyAni` that tips cl into homing a whole callee-saved zero register
(`push ebx / xor ebx,ebx`), shifting every frame offset in the function and
holding it at 49.3% — see
[redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
for the same mechanism read from the other end.

Sites: `??0CAniCycle` 0xaad20 (94.25, base 94 / target 95 instructions),
`??0CFrontCandyAni` 0xacf40 (49.32, 95 / 96), `??0CEyeCandy` 0xac620 (97.85,
124 / 125).

related: [instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md)
(this is a count mismatch that is NOT yet steerable — the exception that proves
the rule; state it, don't file it as regalloc).
