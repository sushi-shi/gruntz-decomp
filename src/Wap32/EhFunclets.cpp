// EhFunclets.cpp - MSVC /GX exception catch/unwind funclets the optimizer emits
// as out-of-line code for a parent function's try/object-cleanup. The delinker
// carves each as its own symbol (it has its own RVA), but no plain C++ source
// produces a standalone, frameless routine that reads the parent's [ebp-N]
// locals and returns a mid-parent continuation address. They are reconstructed
// verbatim as __declspec(naked) bodies: every call/global is an external symbol
// (reloc-masked rel32/DIR32), every continuation a DATA extern (reloc-masked).
#include <Win32.h>
#include <rva.h>

extern "C" {
// The shared CObject-style delete helper (0x1bfe15, __thiscall, ecx=this).
void EhObjDelete();
// Per-funclet engine callees reached by the catch/unwind code (reloc-masked).
void EhHelper1b9b32();
void EhHelper121ca0();
void EhHelper1bb2b7();
void EhHelper1c013b();
// The SetEvent IAT slot the connect-wait funclet pokes (reloc-masked DIR32).
void* g_ehSetEvent;
// Mid-parent continuation addresses (reloc-masked DATA externs).
char g_ehCont_11ec54;
char g_ehCont_1ca0cc;
char g_ehCont_1baae7;
char g_ehCont_1c119f;
char g_ehCont_1bd9ad;
char g_ehCont_1bffc3;
}

// 0x11ec46 - destroy a local (ebp-0x14), resume.
RVA(0x0011ec46, 0xe)
__declspec(naked) void Funclet_11ec46() {
    __asm {
        mov ecx, dword ptr [ebp - 14h]
        call EhObjDelete
        mov eax, offset g_ehCont_11ec54
        ret
    }
}

// 0x1ca116 - destroy a local (ebp-0x18), resume.
RVA(0x001ca116, 0xe)
__declspec(naked) void Funclet_1ca116() {
    __asm {
        mov ecx, dword ptr [ebp - 18h]
        call EhObjDelete
        mov eax, offset g_ehCont_1ca0cc
        ret
    }
}

// 0x1baad2 - destroy a local (ebp-0x24), then arm (ebp-0x18)->+0x2c = -1, resume.
RVA(0x001baad2, 0x15)
__declspec(naked) void Funclet_1baad2() {
    __asm {
        mov ecx, dword ptr [ebp - 24h]
        call EhObjDelete
        mov eax, dword ptr [ebp - 18h]
        or dword ptr [eax + 2Ch], 0FFFFFFFFh
        mov eax, offset g_ehCont_1baae7
        ret
    }
}

// 0x1c1188 - dispatch the queued local (ebp-0x18), then a 2-null cleanup, resume.
RVA(0x001c1188, 0x17)
__declspec(naked) void Funclet_1c1188() {
    __asm {
        push dword ptr [ebp - 18h]
        call EhHelper1b9b32
        push 0
        push 0
        call EhHelper121ca0
        mov eax, offset g_ehCont_1c119f
        ret
    }
}

// 0x1bd98f - virtual-dispatch (slot +0x18) on the local (ebp-0x1c), delete it, resume.
RVA(0x001bd98f, 0x1e)
__declspec(naked) void Funclet_1bd98f() {
    __asm {
        mov esi, dword ptr [ebp - 1Ch]
        push 0F108h
        push 30h
        mov ecx, esi
        mov eax, dword ptr [esi]
        call dword ptr [eax + 18h]
        mov ecx, esi
        call EhObjDelete
        mov eax, offset g_ehCont_1bd9ad
        ret
    }
}

// 0x1bff99 - tear down a stack object (ebp-0x50), signal the arg's event, cleanup, resume.
RVA(0x001bff99, 0x2a)
__declspec(naked) void Funclet_1bff99() {
    __asm {
        lea ecx, dword ptr [ebp - 50h]
        call EhHelper1bb2b7
        mov eax, dword ptr [ebp + 8]
        push dword ptr [eax + 10h]
        mov dword ptr [eax + 18h], 1
        call dword ptr [g_ehSetEvent]
        push 0
        push 0FFFFFFFFh
        call EhHelper1c013b
        mov eax, offset g_ehCont_1bffc3
        ret
    }
}
