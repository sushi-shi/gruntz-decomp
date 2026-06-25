# `new T`-with-throwing-ctor: the ctor-in-flight unwind funclet is appended to the body, not section-split
tags: cpp:new cpp:ctor cpp:eh | asm:call asm:jmp asm:ret | topic:wall topic:eh
symptoms: a `new T` factory whose element has a non-trivial (throwing) ctor matches the /GX EH-framed body 100% up to the `ret`, then the recompile emits an EXTRA out-of-line cleanup funclet (`mov eax,[ebp-N]; push eax; call <op-delete/~T>; ret; mov eax,0; jmp ...`) right after the function; the retail-delinked byte range ends at the `ret` (nop-padded), so the appended funclet is pure residue, ~95-97%
confidence: 7/10
variants: rezalloc-placement-new-no-eh-frame.md, gx-state-machine-scalar-delete-cleanup.md

Modeling a `T* p = new T;` factory where `T` has a non-trivial, potentially-throwing
constructor (vptr stamps + a base/member init call) DOES emit the retail
ctor-in-flight `/GX` frame — `push -1 / fs:0`, the `[esp+N]` trylevel chain
(0 during the ctor, 1 after, -1 on leaving the guarded region), and the
half-constructed-object cleanup edge. The whole **body** lowers byte-identically to
retail, including the `new`-merge null shape and the failure-path scalar-deleting
dtor dispatch (a declared-only polymorphic VIEW cast gives the `mov ecx,el; call
[vtbl+4]` `__thiscall` form).

```cpp
struct ElemBase {                          // the CObject grand-base at +0
    ElemBase() { *(void**)this = &g_baseDtorVtbl; ((ElemBase*)((char*)this+8))->Init(); }
    void Init();                           // the throwing base init (reloc-masked)
    void* m_vptr; i32 m_04; char m_pad08[0x14];
};
struct Elem : public ElemBase {            // the heap element
    Elem() { *(void**)this = &g_elemVtbl; m_04 = 0; m_1c = 0; }
    ~Elem();                               // declared-only -> keeps the new's cleanup edge
    i32 m_1c;
};
Elem* el = new Elem;                       // <- emits the retail ctor-in-flight /GX frame
```

The residue is purely the **unwind funclet placement**: MSVC5 here appends the
exception-cleanup landing pad (destroy the partial object + `operator delete`) right
after the function body, whereas retail carries it in a separate funclet area, so the
delinked function range stops at the `ret`. The appended bytes are uncounted residue
that no source spelling moves.

```asm
; target (delinked range)                   ; our recompile (body identical, then:)
... ret 8 / nop nop nop  (size 0xdd)         ... ret 8 / nop nop  THEN the funclet:
                                             mov eax,[ebp-0x10]; push eax; call <~Elem>
                                             ret;  mov eax,0;  jmp <unwind-cont>
```

WALL (EH-funclet placement). The body is byte-exact; only the trailing unwind
funclet is appended rather than section-split — not source-steerable. Evidence:
`CDDrawSubMgrAni::CreateAniEntry_1528d0` (0x1528d0) — body byte-identical to the
`ret`, 96.87%. Distinct from rezalloc-placement-new (frame ABSENT, ~47%): here the
frame is PRESENT and matches; only the funclet tail differs.
