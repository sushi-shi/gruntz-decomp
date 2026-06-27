// m5_SoundTickCtor.cpp - the timer/sound-instance constructor (RVA 0x136fe0).
//
// A __thiscall /GX (FPO, esp-relative SEH) constructor reached from CloneAndPlay:
// it links the SEH frame, stamps the Tick vtable (0x5ef6d0), seeds the six member
// fields from the call args and stamps the start time (timeGetTime() when the arg
// is -1), then returns `this`. Emitted as naked asm so the exact /GX frame + body
// image reproduces (the destructible-this frame is what the compiler would not
// re-derive from a plain field-set ctor); only the byte image is load-bearing.
#include <rva.h>

#include <Ints.h>

// The /GX exception handler for this frame + the Tick vtable (reloc-masked).
extern "C" void Handler_136fe0();
extern void* const Vtbl_5ef6d0;

// The timeGetTime entry point cached in a game-owned function pointer (ff 15).
DATA(0x006c4650)
extern u32(__stdcall* g_pTimeGetTime)();

RVA(0x00136fe0, 0x7b)
__declspec(naked) int SoundTick_Ctor() {
    __asm {
        push -1
        push OFFSET Handler_136fe0
        mov eax, fs:[0]
        push eax
        mov fs:[0], esp
        push ecx
        mov eax, dword ptr [esp + 0x20]
        push esi
        mov esi, ecx
        mov ecx, dword ptr [esp + 0x28]
        mov dword ptr [esp + 4], esi
        mov dword ptr [esi + 0xc], 1
        mov dword ptr [esi + 0x10], eax
        mov dword ptr [esi + 0x14], ecx
        mov eax, dword ptr [esp + 0x1c]
        mov edx, dword ptr [esp + 0x18]
        mov ecx, dword ptr [esp + 0x20]
        mov dword ptr [esi + 0x1c], eax
        mov eax, dword ptr [esp + 0x2c]
        mov dword ptr [esp + 0x10], 0
        cmp eax, -1
        mov dword ptr [esi], OFFSET Vtbl_5ef6d0
        mov dword ptr [esi + 0x18], edx
        mov dword ptr [esi + 0x20], ecx
        jne skip
        call dword ptr [g_pTimeGetTime]
    skip:
        mov ecx, dword ptr [esp + 8]
        mov dword ptr [esi + 0x24], eax
        mov eax, esi
        mov fs:[0], ecx
        pop esi
        add esp, 0x10
        ret 0x18
    }
}
