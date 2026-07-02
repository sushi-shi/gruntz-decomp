// FreeLibHostDtor.cpp - the FreeLibrary-owning helper destructor body (RVA
// 0x1d4a18), part of the 0x1bf*-0x1d5* COM/registry helper module.
//
// The retail function's true entry is 0x1d4a0d (`mov eax,OFFSET handler; call
// __EH_prolog; push ecx`), so the delinker's 0x1d4a18 boundary lands on the body
// AFTER the EH prologue - the 60 bytes that re-stamp the derived dtor vtable, call
// the owned cleanup fnptr (+0x10) with 0, FreeLibrary the owned HMODULE (+0x08),
// then restore the base vtable and unlink the SEH frame. Emitted as naked asm so
// those exact body bytes reproduce at the carved boundary (ebp/fs references are
// fed by the elided EH prologue; only the byte image is load-bearing).
#include <rva.h>

#include <Win32.h>

// The derived/base scalar-deleting-destructor vtables stamped on entry/exit
// (reloc-masked DATA symbols).
extern void* const Vtbl_5eb6c4;
extern void* const Vtbl_5ec26c;

// The FreeLibrary IAT entry (retail calls it indirectly: ff 15 [0x6c3fdc]).
extern int(__stdcall* g_impFreeLibrary)(void*);

RVA(0x001d4a18, 0x3c)
__declspec(naked) int FreeLibHost_Dtor() {
    __asm {
        push esi
        mov esi, ecx
        mov dword ptr [ebp - 0x10], esi
        mov dword ptr [esi], OFFSET Vtbl_5eb6c4
        mov eax, dword ptr [esi + 0x10]
        and dword ptr [ebp - 4], 0
        test eax, eax
        je skip1
        push 0
        call eax
    skip1:
        mov eax, dword ptr [esi + 8]
        test eax, eax
        je skip2
        push eax
        call dword ptr [g_impFreeLibrary]
    skip2:
        mov ecx, dword ptr [ebp - 0xc]
        mov dword ptr [esi], OFFSET Vtbl_5ec26c
        mov fs:[0], ecx
        pop esi
        leave
        ret
    }
}
