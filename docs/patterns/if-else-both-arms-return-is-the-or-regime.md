# `if (X) { ... return K; } else { return K; }` IS `||` — the TOTAL exit-merge regime
tags: cpp:branch cpp:if cpp:return cpp:goto | asm:xor asm:jcc asm:ret | topic:codegen-idiom
symptoms: every `return 0` in the function collapses onto ONE sunk epilogue even though the source uses separate `if (c) return 0;` guards; base ret count far below the target's; retail gives its entry guards their own inline `xor eax,eax` + pops + `ret N` and the base does not; the exit-merge sieve reports OVER-MERGE but no `||` or `&&` is visible in the source
confidence: 9/10
variants: goto-fail-shares-one-exit-block.md

The three exit-merging regimes in
[goto-fail-shares-one-exit-block](goto-fail-shares-one-exit-block.md) are selected
by the source construct that reaches `return K;`. There is a **fourth spelling of
the TOTAL regime** that does not look like one, and it silently swallows every
other guard in the function:

```cpp
if (X) {
    if (Y) { return 0; }      // nested return
} else {
    return 0;                 // else-arm return of the SAME value
}
```

**This is byte-for-byte the same codegen as `if (!X || !Y) return 0;`** and
therefore the TOTAL regime: *every* same-valued `return` in the whole function
collapses into one sunk block, including entry guards written as plain separate
`if`s a hundred lines earlier.

## Measured (standalone TU, `cl /nologo /c /O2 /MT /GX /GR`)

Four cells with an identical head (two plain entry guards + a loop + a third
guard) differing only in the shape of the LAST guard pair:

| cell | last-guard shape | rets | regime |
|---|---|---|---|
| A | `if (X) { if (Y) return 0; } else { return 0; }` | **2** | TOTAL |
| B | two flat sequential `if (c) return 0;` | **6** | none |
| C | `if (!X \|\| !Y) return 0;` | **2** | TOTAL |
| D | one trailing guard only | 5 | none |

Cells **A and C are identical instruction-for-instruction** (63 insns, verified
after masking branch displacements). Cell B is the no-merge floor.

## Why it matters

The shape is easy to write while transcribing, reads as innocuous nesting, and
its cost lands on *unrelated* guards elsewhere in the function — so the symptom
(entry guards missing their inline epilogues) appears nowhere near the cause.

`CTriggerMgr::ScanGroup` 0x7a760 sat at 89.48 with all five of its `return 0`
sites written as separate `if`s; the single `else { return 0; }` at the far end of
the body was collapsing all of them onto one epilogue.

## The fix

Flatten it, then pick the regime you actually want. Retail's guarded
serialise/init functions are usually PARTIAL — entry guards inline, the deep
sites sharing one block — which is `goto fail;`:

```cpp
i32 hasOv;                          // hoist: a goto may not skip an initialisation
...
if (ar == NULL)  { return 0; }      // entry guards keep their own copies
if (lvl == NULL) { return 0; }
...
    if (obj == NULL) { goto fail; } // the deep sites share ONE block
...
hasOv = (m_overlay != NULL) ? 1 : 0;
ar->Write(&hasOv, sizeof(hasOv));
if (m_overlay != NULL) {
    if (m_overlay->Serialize(ar) == 0) { goto fail; }
}
...
return 1;
fail:
    return 0;
```

`ScanGroup` 89.48 -> **99.19** with 31/31 blocks and 20/20 branch targets
agreeing. The residue is instruction scheduling only.

## It is also a correctness-bug detector

Flattening exposed a real reconstruction bug in the same function: retail's
`m_overlay == NULL` path does **not** return 0, it skips the `Serialize` call and
falls into the success tail (`--blocks --diff` showed base `jcc <fail>` where
retail has `jcc <success>`). The `else { return 0; }` would have failed every
overlay-less save. Check the guard's semantics against the target blocks before
assuming the shape is only a codegen issue.

## Screening

`python -m gruntz.audit.exit_merge_sieve --over` (base rets < target rets), then
read the source for this shape — it will not grep as `||`.

related: [goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md),
[retail-duplicates-small-return-epilogues.md](retail-duplicates-small-return-epilogues.md),
[trailing-statement-blocks-arm-tail-sink.md](trailing-statement-blocks-arm-tail-sink.md)
