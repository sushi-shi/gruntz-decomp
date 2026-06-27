# Standalone /GX EH catch/unwind funclet — reconstruct verbatim as __declspec(naked)
tags: cpp:eh cpp:naked | asm:mov asm:call asm:ret | topic:codegen-idiom
symptoms: tiny carved symbol reads `[ebp-N]` with NO `push ebp` prologue, ends `mov eax,<mid-parent addr>; ret`; `push <addr-just-past-self>; call atexit; pop ecx; ret`; FileNotFoundError .ll from labels.py on `__declspec(naked)` member
confidence: 9/10

The /GX optimizer emits a try/object-cleanup region's catch & state-unwind code
OUT OF LINE, and the delinker carves each as its own symbol (it has its own RVA).
No plain C++ produces a standalone, frameless routine that reads the *parent's*
`[ebp-N]` locals and returns a mid-parent continuation address — so there is no
"real source". Transcribe the bytes as a `__declspec(naked)` **free** function
(naked is rejected on member functions by clang's label pass — make it free, it
still pairs by RVA): every `call`/global is an `extern` symbol (reloc-masked
rel32/DIR32), every continuation a `DATA` extern (reloc-masked DIR32). The same
applies to CRT static-destructor `atexit` stubs (whose 1-dword cleanup is
`pop ecx`, which cl /O2 will not emit from `atexit(fn)` — it picks `add esp,4`).

```cpp
extern "C" void EhObjDelete();            // rel32 callee (reloc-masked)
extern "C" char g_ehCont_11ec54;          // DATA continuation (reloc-masked DIR32)
RVA(0x0011ec46, 0xe)
__declspec(naked) void Funclet_11ec46() {
    __asm {
        mov ecx, dword ptr [ebp - 14h]    // parent local
        call EhObjDelete
        mov eax, offset g_ehCont_11ec54   // mid-parent resume addr
        ret
    }
}
```
```asm
0011ec46: 8b 4d ec        mov ecx,[ebp-0x14]
0011ec49: e8 .. .. .. ..  call <dtor>          ; REL32 reloc
0011ec4e: b8 .. .. .. ..  mov eax,<continuation>; DIR32 reloc
0011ec53: c3              ret
```
Steerable → 100% exact. Evidence: 6 funclets 0x11ec46/0x1ca116/0x1baad2/0x1c1188/
0x1bd98f/0x1bff99 + the atexit trio 0x1ce973/0x1d4239/0x1d4c16 all byte-exact;
the `and [ecx],0` detach 0x1c0a41 and the x87 whole-number classifier 0x18c1f5
(result built in `cl`) use the same naked escape hatch.
