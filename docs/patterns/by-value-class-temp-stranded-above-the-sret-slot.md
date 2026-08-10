# A by-value class temp is STRANDED when a struct-returning call feeds the next call's argument
tags: cpp:call cpp:class cpp:temp | asm:push asm:call | topic:correctness topic:codegen-idiom

symptoms: two or more `push` between the by-value copy-ctor `call` and the call that consumes
the temp; a struct-returning `__thiscall` that reads its class parameter as an unrelated int;
`EXCEPTION_ACCESS_VIOLATION` reading `<small int> - 8` inside a `CString` parameter's
`GetLength()`; the same statement in retail issues only ONE `push` (the hidden sret pointer)
between the ctor and the call

confidence: 10/10

cl 5.0 materializes a by-value class argument IN PLACE in its outgoing-argument slot
(`push <junk>` to reserve, `mov ecx,esp`, copy-ctor). For a struct-returning callee the hidden
sret pointer must be pushed IMMEDIATELY after, so the temp abuts it. When the struct-returning
call's RESULT feeds an argument of the *next* call, and nothing between them is itself a call,
cl hoists that next call's trailing pushes ABOVE the struct-returning call — stranding the
already-constructed temp two slots too high. The callee then reads the hoisted int as its class
parameter and the outer call receives its arguments shifted by one.

```cpp
// MISCOMPILES - Measure() receives rc.top, not `line`:
i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
DrawLine(line, surf, cx, rc.top, z);

// CORRECT - binding the result ends the temp's slot before the outer call's pushes:
TextExtent le = MeasureText(line);
i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
DrawLine(line, surf, cx, rc.top, z);
```

```asm
; BROKEN (base): the CString temp lands at [S], MeasureText reads arg2 at [S-8] = edi
push  ecx                       ; reserve the temp slot  -> [S]
mov   ecx,esp
push  edx
call  ??0CString@@QAE@ABV0@@Z
push  eax                       ; hoisted DrawLine arg (z)
push  edi                       ; hoisted DrawLine arg (rc.top)   <-- READ AS THE CString
lea   ecx,[esp+0x4c]
push  ecx                       ; sret
call  ?MeasureText@FontRenderer@@QAE?AUTextExtent@@VCString@@@Z   ; ret 8

; CORRECT (retail, and after the fix): exactly ONE push between ctor and call
push  ecx
mov   ecx,esp
push  eax
call  ??0CString@@QAE@ABV0@@Z
lea   ecx,[esp+0x3c]
push  ecx                       ; sret
call  ?MeasureText@...
```

STEERABLE, and a hard correctness bug — not a scoring artifact. Reproduced standalone with
cl 5.0 `/O2 /MT /GX`: the one-liner form miscompiles, `TE m = Measure(line);` and an
out-of-line `Width()` both emit the correct sequence. The tree-wide screen is mechanical —
disassemble the linked candidate, find every `mov ecx,esp` followed within a few instructions
by a `call`, and flag any site with >= 2 `push` before the consuming call (the only legitimate
multi-push case is a callee whose class parameter is its LAST declared parameter, e.g.
`CGruntzMgr::BuildLevelRezPath(int,int,int,int,CString)`). Five real sites existed, all in
`FontRenderer::DrawWrapped`; fixing them took it 73.27 -> 74.29 and removed a guaranteed
runtime crash on every centered wrapped-text draw.
