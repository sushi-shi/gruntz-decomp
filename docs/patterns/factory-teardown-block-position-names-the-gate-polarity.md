# Where the `delete` block sits names the gate polarity of a `new`-factory
tags: cpp:new cpp:guard cpp:eh | asm:jne asm:je | topic:codegen-idiom
symptoms: `walls jccscan` reports one je/jne complement flip on a factory whose
body is `p = new T; ... if (gate) { delete p; return NULL; }`; base and target
disagree only in which arm falls through
confidence: 8/10

For a `new`-factory guarded on one condition, cl 5.0 lays out the THEN block as
the fallthrough. So the retail position of the `delete` block reads the source
polarity straight off, with no ambiguity:

* `delete` block INLINE right after the compare, success block jumped-to
  => the source is the negative early return, `if (bad) { delete p; return NULL; } <success>`
* `delete` block at the END of the function, body falling through
  => the source is the positive gate, `if (ok) { <body>; return p; } delete p; return NULL;`

```asm
; negative early return (CTileTriggerContainer::AddSwitchLogic 0x115f60, 100.00% EXACT)
  test eax,eax
  jne  <success>            ; success is jumped to
  push esi                  ; teardown INLINE
  mov  DWORD PTR [esi],??_7CTileTriggerSwitchLogic@@6B@
  call ??3@YAXPAX@Z

; positive gate (CTileTriggerContainer::AddToList3 0x116a40)
  mov  eax,DWORD PTR [esi+0x10]
  test eax,eax
  jne  <teardown at the end>
  <body falls through>
```
STEERABLE. Used to fix `CTileTriggerContainer::AddToList1` 0x116cf0 and
`::AddToList3Switch` 0x116b80, which retail lays out with the teardown last;
both were written with the negative early return and emitted the teardown
inline. Caution: the correction is not free - it moves the null-allocation
return from its own `xor eax,eax`+`jmp` into the shared epilogue (our teardown
lands before the epilogue, retail's after), so AddToList3Switch fell 75.78 ->
72.30 while gaining the correct arm order. The remaining flip in all three
`AddToList*` is that epilogue placement, not the gate.
