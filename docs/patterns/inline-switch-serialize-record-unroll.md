# Unrolled serialize records: `__inline` helper + `switch(mode)`, not `if/else`
tags: cpp:inline cpp:switch cpp:method | asm:call asm:cmp asm:je | topic:codegen-idiom
symptoms: a Save/Load that streams N fixed fields emits N IDENTICAL inline blocks each `cmp mode,4; je <write>; cmp mode,7; jne <next>; <read>; <write floated>`; your helper-call recompile shows `call SerRecord` + `add esp,0xc` per field (helper NOT inlined) and/or `cmp;jne` if/else polarity
confidence: 9/10
variants: nested-if-success-deepest-error-tail.md

A polymorphic-archive serializer (`mode 4 = Write, mode 7 = Read` via vtable
slots) that streams a run of same-shape fixed records unrolls each record INLINE
in retail. Two levers, both load-bearing:

1. **`__inline` the per-record helper.** MSVC 5.0 does NOT auto-inline a `static`
   free function at /O2; a plain `static void SerRec(...)` emits `call SerRec` +
   caller-cleanup `add esp,N` at every site (this alone caps the fn ~38–78%).
   The MSVC5 keyword `__inline` forces it (honored at /O2; clang-cl also inlines).
2. **`switch(mode)` inside the helper AND at the mode-dispatch, not `if/else`.**
   A two-case `switch` floats the first-tested case body PAST the second
   (`cmp 4; je WRITE; cmp 7; jne NEXT; READ; jmp NEXT; WRITE: …`) — exactly
   retail. An `if(mode==4){…}else if(mode==7){…}` emits the opposite branch
   polarity/placement (`cmp 4; jne; WRITE inline; …`). The same switch on a sparse
   case set `{4,7,8}` at the outer dispatch lowers to the subtract-cascade
   (`mov eax,mode; sub eax,4; je; sub eax,3; je; dec eax; jne`) with the rare arms
   forward-floated to the tail, where the if/else-cascade keeps inline `cmp;jne`.

```cpp
static __inline void SerRecord(CGruntArchive* ar, i32 mode, char* p) {
    switch (mode) {
    case 4: ar->Write(p, 8); ar->Write(p + 8, 8); break;   // save
    case 7: ar->Read(p, 8);  ar->Read(p + 8, 8);  break;   // load
    }
}
// dispatch: switch(mode){ case 4: …; case 7: …; case 8: …; }  (NOT if/else)
SerRecord(ar, mode, (char*)this + 0x810);   // inlined per record
SerRecord(ar, mode, (char*)this + 0x820);
// …
```
```asm
; per record, retail (mode in edi):
cmp edi,4 ; je  WRITE
cmp edi,7 ; jne NEXT
; READ:  mov edx,[esi]; push 8; push ebp; mov ecx,esi; call [edx+0x2c]; …
; WRITE: mov edx,[esi]; push 8; push ebp; mov ecx,esi; call [edx+0x30]; …
```

Evidence: `CGrunt::SerializeMove` (0x53b80, 832 B) — helper-call + if/else =
38.8%; `__inline` helper = 77.6%; + outer `switch` = 90.8%; + in-helper `switch`
= 100.0% byte-exact.
