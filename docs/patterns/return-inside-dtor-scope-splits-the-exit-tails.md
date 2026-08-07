# `return` inside a destructor scope splits two exit tails that `break`/`goto` merge

tags: cpp:branch cpp:return cpp:eh cpp:local | asm:jmp asm:call | topic:codegen-idiom topic:tail-merge
symptoms: a `/GX` scope holding one destructible local has two conditional exits;
  `--blocks --diff --lite` shows retail with TWO full copies of the destructor tail
  (`5i [jmp Bk]` then `4i [jmp Bk]`, differing only by the leading EH-state store)
  while the base has `1i [fall Bk]` + `4i [jmp Bk]` - i.e. the first exit stores the
  EH state and falls into the second exit's destructor call. `insn_seq --seq` names
  the whole missing destructor call run (`+ call AfxGetModuleState / + call
  EndWaitCursor`).
confidence: 7/10

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

**THE CATCH, and why this is not yet a free win.** Removing the loop's normal exit also
removes a live range: in `StartUpPrompt` @0x1f9b0 retail pins the `HWND` parameter in
`ebx` across the loop, which leaves cl no callee-saved register for the `-1` the inline
`strcpy` needs, so retail spells that `or ecx,0xffffffff` (1 insn). With the `return`
form cl has `ebx` free, hoists the constant into it (`or ebx,-1; mov ecx,ebx`) and pays
+1 insn at each of the two inline `strcpy` sites, and the `MessageBoxA` receiver becomes
`mov eax,[esp+..]; push eax` instead of `push ebx`. Measured on that function:

| spelling | insn delta | fuzzy |
|---|---|---|
| `break` x2 (or `goto` x2) | -4 (destructor tail merged) | 97.99 |
| `return 1` x2 | +2 (the two hoisted `-1`s) | 95.97 |

Forcing the parameter back into a register does not work: an `HWND` local scoped to the
branch, or one shared by both `MessageBoxA` call sites, leaves the allocation unchanged
(both measured). So the two knobs are currently mutually exclusive; the `break` form is
banked and the `return` form is the shape to re-try once the register pressure moves.

related: statement-order-decides-the-tail-merge.md, identical-arms-need-distinct-locals.md,
identical-return-epilogue-tailmerge.md
