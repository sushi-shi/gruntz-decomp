# Reconstruct a carved EH funclet / mid-function fragment as a naked __asm body
tags: cpp:eh cpp:naked | asm:call asm:jmp asm:mov | topic:codegen-idiom topic:eh
symptoms: a standalone "function" target that is really a compiler EH funclet (destructor-unwind, catch block, /GX frame teardown tail `mov fs:[0],ecx;leave;ret N`), an atexit static-dtor stub (`push <addr>;call atexit;pop ecx;ret`), or a multi-entry shared-tail head (`...;jmp short <outside>`) — no standalone C++ source form
confidence: 9/10

Ghidra carves C++ EH funclets (and other mid-function fragments) as their own
"functions", so the delinker emits a per-symbol target obj for each. They have no
prologue and no standalone C++ source — they are pieces of a parent's /GX state
machine. Reconstruct each as a `__declspec(naked) void f()` whose `__asm` body is
the literal instruction stream. Every address operand resolves to an **external
no-body symbol** so its relocation is masked by objdiff (code bytes match):
`call Extern` → reloc-masked `e8 rel32`; `mov reg, OFFSET DataExtern` →
reloc-masked `b8/bb … DIR32`; `push OFFSET FnExtern` → `68 DIR32`. `mov fs:[0],ecx`
/ `mov eax,fs:[0]` and `_emit 0xNN` (for a 2-byte short `jmp` whose target is
outside the carved range) are literal. Flags = `base` (naked needs no `/GX`).

```cpp
extern "C" void Delete_1bfe15();      // thiscall callee — set ecx in asm
extern int Cont_11ecd6;               // continuation addr (mov OFFSET — data extern OK)
__declspec(naked) void Unmatched_11ecc8() {
    __asm {
        mov ecx, dword ptr [ebp+8]
        call Delete_1bfe15
        mov eax, OFFSET Cont_11ecd6   // b8 <DIR32 reloc-masked>
        ret
    }
}
```
```asm
8b 4d 08 / e8 <rel32> / b8 <DIR32> / c3      ; destructor-unwind funclet
```
STEERABLE → 100%. GOTCHA: clang's MS-asm parser (the label/IR step) **segfaults on
`push OFFSET <data-extern>`** — declare a pushed label as a *function* (`extern "C"
void Lbl();`) not `extern int`; `mov reg, OFFSET <data-extern>` is fine. Evidence:
the m1_ehfunclets unit (11 fns: dtor-unwind 0x11ecc8/0x1ca2c3, /GX teardown tails
0x11ec00/0x1c744f, catch funclets 0x1c11a5/0x1ca095, atexit stubs 0x1d3184/0x1d4271,
shared-tail head 0x11f23d via `_emit 0xeb 0x0a`) + FaderDtor 0x17f570 (a full /GX
dtor whose real-C++ form caps at the vptr plateau) — all 100%.
