# A gate that always `return`s lets cl DELETE a later re-test of the SAME storage

- **confidence** c9
- **tags** `cpp:branch` `cpp:switch` `cpp:dce` | `asm:cmp` `asm:jne` | `topic:codegen-idiom`
- **measured** `CGrunt::ScanNearestTarget` @0xf42f0 **52.70 -> 64.78** on this one line
  (instruction deficit -210 -> -69): a whole `case` arm came back.

## Symptom

`insn_seq` says a big NEGATIVE deficit and the reloc-name histogram shows one
complete cluster of callees each short by exactly one:

```
?CommitNeighbor@CGrunt@@QAEHHHHH@Z    2 -> 3
?GruntInRadius@CGrunt@@QAEHHH@Z       2 -> 3
?RectContains@CGrunt@@QAEHHH@Z        3 -> 4
jmpl (jump-table dispatch)           10 -> 12
```

One arm's worth of *everything*. In the base obj the arm is not misplaced, it is
**absent**: the dispatch falls straight through to the arm's failure tail.

```asm
; base                                    ; target
  decl %eax                                 dec  eax
  movl $0x1, %eax                           jne  <return 1>
  jne  <return 1>                           cmp  ecx,ebp        ; m_poweredUp
  movl %eax, 0x2d4(%ebx)   ; the FAILURE    jne  <the arm body> ; <- survives
  movl $0x1f4, 0x2ec(%ebx) ; tail, inline
```

## Cause

cl5 /O2 propagates "this storage is 0 here" out of a guard **all of whose exits
`return`**:

```cpp
if (m_poweredUp != 0) {
    ...                       // every path returns
    return 1;
}
switch (m_defenderState) {
    case AISTATE_ATTACK:
        if (m_poweredUp != 0) {   // cl proves this is FALSE and deletes the body
            ...
        }
```

Retail emits both tests. The difference is **which storage each one reads**:
retail's gate keeps the value in ONE register across the gate, the two stamina
arms and the switch arm (`mov ecx,[ebx+0x220]` once, then `cmp ecx,ebp` three
times) — that is a **local**; the switch arm's test is a **member** load. cl
propagates the fact within one storage kind, not across the two.

## Fix

Read the gate through a local and leave the later re-test on the member:

```cpp
i32 powered = m_poweredUp;     // the gate - one cached load, like retail
if (powered != 0) {
    ...
    return 1;
}
switch (m_defenderState) {
    case AISTATE_ATTACK:
        if (m_poweredUp != 0) {   // the member - cl cannot fold it, the arm survives
```

Both halves are needed. `powered` in both places still deletes the arm (measured
52.70, arm gone); `m_poweredUp` in both places also deletes it (51.15, arm gone);
only the local/member split reproduces retail.

This is NOT a licence to sprinkle locals: the local is what the *disassembly*
shows (one load, three compares off one register). Read the register first.

related:
[instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md),
[mnemonic-histogram-diff-finds-the-wrong-idiom.md](mnemonic-histogram-diff-finds-the-wrong-idiom.md),
[dead-unreachable-recheck-block-dce.md](dead-unreachable-recheck-block-dce.md)
