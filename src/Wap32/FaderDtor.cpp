// FaderDtor.cpp - 0x17f570, the /GX destructor of a CFader-derived class.
//
// It is the documented EH-frame + vptr-stamp + member-teardown shape: a real
// polymorphic /GX destructor caps at the vptr-store plateau because the emitted
// vtable would diverge from retail (the class's virtuals are not all modeled).
// Reconstructed instead as a naked __asm body whose bytes are literal; the EH
// handler address, the retail vtable address (0x5f07f8), the RezFree call and the
// base ~CFader call all resolve to external no-body symbols and are reloc-masked.
#include <rva.h>

extern "C" void Handler_1e3748();     // 0x5e3748 - __ehhandler$ (push OFFSET needs a fn sym)
extern void* const Vtbl_5f07f8;       // 0x5f07f8 - this class's vtable
extern "C" void RezFree(void*);       // 0x1b9b82
extern "C" void DtorCFader_17e4a0();  // 0x17e4a0 - base ~CFader

RVA(0x0017f570, 0x61)
__declspec(naked) void Dtor_17f570() {
    __asm {
        push 0xffffffff
        push OFFSET Handler_1e3748
        mov eax, fs:[0]
        push eax
        mov fs:[0], esp
        push ecx
        push esi
        mov esi, ecx
        mov dword ptr [esp+4], esi
        mov dword ptr [esi], OFFSET Vtbl_5f07f8
        mov eax, dword ptr [esi+0x4c]
        mov dword ptr [esp+0x10], 0
        test eax, eax
        je skip
        push eax
        call RezFree
        add esp, 4
        mov dword ptr [esi+0x4c], 0
    skip:
        mov ecx, esi
        mov dword ptr [esp+0x10], 0xffffffff
        call DtorCFader_17e4a0
        mov ecx, dword ptr [esp+8]
        pop esi
        mov fs:[0], ecx
        add esp, 0x10
        ret
    }
}
