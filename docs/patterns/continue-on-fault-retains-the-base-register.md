# Two fields holding ONE garbage pointer is a stepped-over load, not aliased data — the value is the BASE, not the element
tags: cpp:class cpp:local | asm:mov | topic:runtime topic:identity
symptoms: two different array indices, or two unrelated struct fields, observed holding the
same impossible pointer; a `CObArray`/`CPtrArray` element that equals what the container's
own data pointer should be; a fault burst of N consecutive reads inside one call.
confidence: 9/10

wine's `krnl386.exe16` handler returns `EXCEPTION_CONTINUE_EXECUTION` and **steps the
faulting instruction over**, so a load that faults never writes its destination — the
register keeps what it held a moment earlier. For the two-instruction indexed-read idiom
cl 5.0 emits, that earlier value is the BASE pointer, and every index collapses onto it:

```asm
    mov  edx,DWORD PTR [ecx+0x14]   ; edx = container.m_pData        <- survives
    mov  edx,DWORD PTR [edx+0x4]    ; edx = m_pData[1]   FAULTS -> edx still m_pData
    ...
    mov  edx,DWORD PTR [ecx+0x14]   ; edx = container.m_pData
    mov  edx,DWORD PTR [edx+0x8]    ; edx = m_pData[2]   FAULTS -> edx still m_pData
```

Both stores therefore land the SAME value, and that value is `m_pData`. Reading it as two
aliased elements poses a question with no answer: `CDDrawWorker::InsertFrame` (0x151f00)
refuses to overwrite a non-null slot (`cmp DWORD PTR [eax+edi*4],ebp; je alloc`) and every
frame path allocates a fresh `CImage`, so no correct fill can put one pointer at two
indices. The observation constrains the CONTAINER, not its contents.

Corroborate with the burst shape: the same run's `LayerBlitFrame+0x6a,+0x7f,+0x86,+0x8b,+0x90`
is five consecutive faulting field reads inside ONE call, which is only possible if each is
stepped over rather than retried — the same evidence that licenses the register-retention
reading. Worked case: `CPlay::LoadLoadingBarSprite` 0xd7440 writing `m_revealCapStart`
(+0x4c8) and `m_revealCapMid` (+0x4c0) — both read back 0x01e2b3a0, which is
`m_items.m_pData`, not `GetAt(1)`/`GetAt(2)`.
