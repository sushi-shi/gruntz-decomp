# Retail emits an unreachable dead BLOCK our MSVC5 DCE eliminates

tags: cpp:branch cpp:switch cpp:member | asm:cmp asm:jne | topic:wall topic:regalloc

symptoms: a `switch`/state machine has a `case N:` whose body begins `if (m_flag != 0)
{ <big recheck> } else { <small default> }`, but `m_flag` was already proven 0 by an
early-out ABOVE the switch (`if (m_flag != 0) { ...; return 1; }`). Retail EMITS the
whole `if (m_flag != 0)` recheck block (dozens of insns, reached by `cmp <cached>,0;
jne recheck`); our same MSVC5 cl proves it unreachable and DEAD-CODE-ELIMINATES it, so
the case collapses to just the default. base is tens of insns shorter, per state.

The grunt per-tick AI steps (CGrunt::ChargeStep 0xef6b0, ::UpdateArrival 0xf0130,
CGruntScan::ScanNearestTarget 0xf42f0) all share this shape:

```cpp
if (m_poweredUp != 0) {          // early-out: every path returns 1
    ...
    return 1;
}
switch (m_defenderState) {
case 2:
    if (m_poweredUp != 0) {      // DEAD in-source: m_poweredUp is provably 0 here
        ... 57-78-insn recheck (re-grid, CanReach, AtTile, FaceTarget) ...
    }
    m_defenderState = 1; m_dwell = 0x1f4; return 1;
```

Retail keeps `case 2`'s recheck: `cmp <ecx=cached m_poweredUp>,0; jne 0x198; [rearm]`
with the 0x198 block fully emitted (dead — 0x157/the switch has ONE predecessor: the
`je` taken only when m_poweredUp==0, verified with `grep -n "0x157"` on the disasm).
Our cl runs a stronger unreachable-block pass and drops the recheck entirely.

WALL: this is the control-flow (dead-BLOCK) cousin of [[dead-global-read-spill-dce]]
(a dead-SPILL). No natural C++ spelling forces cl to emit a provably-unreachable block:
the recheck body reads the same member the guard already narrowed, so value-propagation
kills it. `volatile` on the member (cf. [[volatile-member-preserves-redundant-store]])
was NOT tried here — it would also change the guard's codegen. Accept the plateau: the
recheck is ~12-14% of each function, so these cap in the 40-60s even after the
`switch` dispatch + reverse layout are byte-exact. Evidence: ChargeStep 57.6%,
UpdateArrival ~34%, ScanNearestTarget ~28% (all @early-stop, logic + dispatch correct).
