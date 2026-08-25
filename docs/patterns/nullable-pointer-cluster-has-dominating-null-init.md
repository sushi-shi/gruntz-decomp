# A nullable-pointer cluster has one dominating NULL initialization
tags: cpp:pointer cpp:local cpp:branch | asm:xor asm:mov asm:jmp | topic:codegen-idiom
symptoms: base has one assignment/zero/join diamond per pointer while retail has a run of
zero stores before any guards, followed only by conditional non-null assignments
confidence: 9/10

## Symptom

A group of related optional pointers was transcribed as independent ternaries:

```cpp
T* up = hasUp ? p - stride : NULL;
T* down = hasDown ? p + stride : NULL;
```

cl 5.0 emits a separate conditional assignment, zero arm, and join for each expression.
When retail instead zeroes every pointer at one dominating point and its guards contain
only the non-null assignments, the source shape was:

```cpp
T* up = NULL;
T* down = NULL;
if (hasUp) {
    up = p - stride;
}
if (hasDown) {
    down = p + stride;
}
```

This is especially visible in neighborhood scans: retail begins with an `xor` and a run
of register/stack zeroing, then each bounds check jumps around only the address-forming
assignment. The ternary reconstruction instead repeats zero arms and short unconditional
jumps throughout the scan.

## Evidence

`CGruntzMapMgr::BuildCellAttributes` at 0x000810f0 had 23/23 calls and 122/122 relocations
after restoring the retail reload of `g_gameReg`, but its eight nullable neighbor pointers
still produced too many blocks. Dominating `NULL` initializers plus conditional assignments,
and branching directly on the four-pair predicate instead of materializing a `bool`, moved
the function from 76.28% to 83.83%. Restoring the signed 3x3 loop bounds and unsigned edge
guards then moved it to 84.76%. The remaining difference is register-home and consequent
loop-layout residue, not a missing call or referent.

## Reverse-use boundary

Apply this only when retail visibly has the dominating zero stores and lacks a zero arm
beside each guarded assignment. It is not a general preference for `if` over `?:`:
[[biased-pointer-advance-ternary]] is a counterexample where the ternary is the proven
retail shape. Predicate materialization is a separate, composable issue described by
[[bool-local-materializes-what-should-be-short-circuit]].
