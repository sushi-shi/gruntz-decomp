# Re-run a ctor in place: explicit `obj.Class::Class()` gives a clean tail-jmp; placement-new adds a null-check
tags: cpp:ctor cpp:new | asm:jmp asm:mov asm:test | topic:codegen-idiom
symptoms: standalone 0xa-byte fn `mov ecx,offset g; jmp Ctor` (no null-check); placement-new `new (&g) T` emits an extra `test eax,eax; je` and scores 0%
confidence: 9/10

A tiny standalone function that constructs a single known global object in place
lowers in retail to a bare `mov ecx,offset g_obj; jmp ??0T@@QAE@XZ` tail-jmp — no
`operator new` call, no null-check. Placement-new (`new (&g_obj) T;`) does NOT
reproduce this: MSVC5 keeps the mandatory placement-new null-guard
(`test eax,eax; je`) even when the placement pointer is `&global` (provably
non-null), giving 12 bytes vs the retail 10 and scoring 0%. Use MSVC5's
explicit-constructor-call extension `g_obj.Class::Class();` instead — it re-runs the
ctor on the lvalue with the plain thiscall tail-jmp and no guard.

```cpp
extern CFileIO g_obj646778;            // the engine's ONE static MFC CFile global
void Forwardb5400() {
    g_obj646778.CFileIO::CFileIO();    // NOT `new (&g_obj646778) CFileIO;`
}
```
```asm
b5400: b9 78 67 64 00   mov  ecx, offset g_obj646778   ; DIR32 reloc-masked
b5405: e9 cd 9b 10 00   jmp  ??0CFileIO@@QAE@XZ        ; REL32 reloc-masked (retail names it "CFileIO")
```
Steerable → exact. Forwardb5400 0xb5400 100% (placement-new gave 0%); lets a shared
file/object global be typed to its REAL class and kills the hex-named view struct.
