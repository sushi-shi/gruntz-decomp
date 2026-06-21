# A `bool` return whose value spans the SEH-frame teardown is re-canonicalized — spell `? true : false`
tags: cpp:eh cpp:branch | asm:test asm:setne | topic:codegen-idiom
symptoms: trailing `test al,al; setne al` present in retail but absent in recompile, interleaved with the `mov fs:[0],ecx` / pop cleanup of a /GX function, otherwise byte-exact
confidence: 8/10

In a `/GX` function whose `return <bool>` value is produced *before* the SEH-frame
teardown (the `mov fs:[0],ecx` restore + the callee-saved pops), MSVC 5.0 re-canonicalizes
the boolean **after** the cleanup: it emits `test al,al; ... setne al` interleaved with the
pop sequence, normalizing whatever `al` holds back to {0,1} once the value has survived the
frame restore. A plain `return ScanToken(...)` (or `return someBool`) does NOT reproduce
this — our `cl` returns `al` directly across the cleanup and is otherwise byte-identical,
leaving a 2-instruction (`test`+`setne`) deletion as the only residue.

STEERABLE. Spell the return as an explicit boolean normalization so the source carries the
canonicalization the dev's `BOOL`/`bool` flow implied:

```cpp
return ScanToken(3) ? true : false;   // emits test al,al; setne al after the SEH teardown
```

`!!expr` / `expr != 0` produce the same `setne`, but a non-bool intermediate
(`int ok = ScanToken(3); return ok != 0;`) spills the temp to a stack slot (`and eax,0xff;
mov [esp+..],eax`) and over-diverges — keep the ternary on the call result directly.

Evidence: CButeMgr::ParseTagLine 97.6% → 100% byte-exact on `? true : false` (the lone
real diff was the missing `test al,al; setne al` in the SEH-cleanup tail; all other rows
were reloc-masked symbol names). Contrast: the non-EH leaf bool getters (GetInt etc.) need
no normalization — this idiom is specific to the bool return crossing the /GX frame restore.
