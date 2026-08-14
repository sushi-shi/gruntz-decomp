# `return` inside a destructor scope splits two exit tails that `break`/`goto` merge

tags: cpp:branch cpp:return cpp:eh cpp:local | asm:jmp asm:call | topic:codegen-idiom topic:tail-merge
symptoms: a `/GX` scope holding one destructible local has two conditional exits;
  `--blocks --diff --lite` shows retail with TWO full copies of the destructor tail
  (`5i [jmp Bk]` then `4i [jmp Bk]`, differing only by the leading EH-state store)
  while the base has `1i [fall Bk]` + `4i [jmp Bk]` - i.e. the first exit stores the
  EH state and falls into the second exit's destructor call. `insn_seq --seq` names
  the whole missing destructor call run (`+ call AfxGetModuleState / + call
  EndWaitCursor`).
confidence: 10/10

cl5 decides tail-merging on the IL STATEMENT list, and `break`/`goto` out of the scope
produce the *same* statement list at both exits (destruct, leave the loop), so the two
destructor tails collapse. Spelling both exits as `return <value>` gives each its own
statement list and cl emits both destructor copies, exactly like retail.

```cpp
// merged - one destructor tail shared by both exits
for (;;) {
    ...
    { CWaitCursor wait;
      if (Ready()) break;
      Sleep(1000);
      if (Ready()) break; }
}
return 1;

// split - each exit gets its own destructor copy, as retail has
for (;;) {
    ...
    { CWaitCursor wait;
      if (Ready()) return 1;
      Sleep(1000);
      if (Ready()) return 1; }
}
```

`goto <label-after-the-loop>` behaves exactly like `break` - measured, it does NOT
split the tails.

An earlier build made the source-correct return form look like a local minimum: it
freed `ebx`, hoisted the inline-`strcpy` `-1` into that register, and paid one extra
instruction at each copy site. That observation was compiler-state-specific, not a
reason to retain the structurally wrong `break` spelling. Re-running the direct A/B in
the current pinned tree gives:

| spelling | insn delta | fuzzy |
|---|---|---|
| `break` x2 (or `goto` x2) | -4 (destructor tail merged) | 97.99 |
| `return 1` x2 | exact | **100.00** |

The current return form also reproduces retail's `HWND`/`ebx` lifetime and both inline
copy sequences without a steering local. This is a negative control for score-led
parking: when the destructor-exit topology proves `return`, retain it across a temporary
register-allocation dip and re-audit after legitimate TU/header reconstruction changes.

related: statement-order-decides-the-tail-merge.md, identical-arms-need-distinct-locals.md,
identical-return-epilogue-tailmerge.md
