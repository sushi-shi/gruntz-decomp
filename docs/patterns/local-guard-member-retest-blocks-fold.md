# Outer guard on a LOCAL + inner re-test on the MEMBER survives cl's branch folding

tags: cpp:local cpp:member cpp:branch | asm:cmp asm:jcc | topic:codegen-idiom
symptoms: a guard battery inside `if (m_x != 0) { if (m_y == 0) { ... } }` is
missing 1-2 conditional branches vs retail on the NO-CALL arm while the
post-call arm matches; retail spells the "missing" tests as bare register
compares (`cmp eax,<zeroreg>`) on values loaded once by the outer guards
confidence: 8/10

cl 5.0 /O2 deletes a member re-test that is dominated by an identical member
guard with no intervening call or escaping store (`if (m_a != 0) { ...
if (m_a == 0) goto out; }` folds to nothing). It also copy-propagates a
singly-assigned local straight back to its member initializer, so spelling
BOTH tests through locals folds identically. The combination it cannot
connect is MIXED: outer guard reads a LOCAL snapshot, inner re-test reads
the MEMBER. The re-test then survives as a register compare (the member's
first load is still cached), which is exactly retail's emission.

```cpp
i32 powered = m_poweredUp;            // outer guards: the LOCALS
if (powered != 0) {
    i32 neighborValid = m_neighborValid;
    if (neighborValid == 0) {
        if (m_stamina >= STAMINA_FULL) {
            if (FindGridNeighbor(1) != NULL) { ... return 1; }
            // post-call arm: member re-tests reload from memory (call kills
            // the cache) - spell them as members, they cannot fold anyway
            if (m_poweredUp == 0 || m_neighborValid != 0) goto retreat;
        } else {
            if (flag != 0) goto retreat;
            // NO-CALL arm: member re-tests here fold if the outer guards
            // are members too; with LOCAL outer guards they survive as
            // cmp <reg>,<zeroreg> - retail's shape
            if (m_poweredUp == 0 || m_neighborValid != 0) goto retreat;
        }
        m_entranceActive = 0; ... ResetEntranceAnimation(1, 0, 0);
    }
}
```

Do NOT spell the inner tests through the locals (copy-prop folds them) and do
not expect member-outer/member-inner to keep them (dominance folds them).

Measured: `CGrunt::SeekTarget` 0xf71c0 80.82 -> 85.14 (the biggest hit; its
file comment had declared the fold "not reachable from source" - retired).
`CGrunt::WanderStep` 0xed9f0 83.38 -> 84.34 (the no-call arm's two
tests reappear as `cmp eax,ebp` / `cmp ecx,ebp`, block census 115 -> 117 of
retail's 121). `CGrunt::ScanNearestTarget` 0xf42f0 carries the same shape
(its `powered`/`neighborValid` locals are why its else-arm tests survive).
The five-store + ResetEntranceAnimation battery recurs across the CGrunt
step/scan family - screen StepDiggerBehavior / StepGooSuckerBehavior /
GruntDefense* for the same missing-branch signature.
