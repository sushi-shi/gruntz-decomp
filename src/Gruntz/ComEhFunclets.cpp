#include <rva.h>
// ComEhFunclets.cpp - compiler-emitted C++ exception funclets carved by the
// delinker as standalone symbols in the engine_unmatched worklist. Each is the
// catch-handler / SEH-unwind funclet of a PARENT function's try/catch (the devs
// wrote try/catch; cl 5.0 /GX emitted these out-of-line funclets). They reference
// the PARENT'S frame slots ([ebp-N]) and return a hardcoded continuation address
// into the parent - so they have NO standalone C++ source form. The build carves
// them as their own RVA symbols (not paired to a parent obj), so the only faithful
// reconstruction is the exact funclet bytes via `__declspec(naked)` __asm.
// Externs reloc-mask; the continuation targets are the delinker's DAT/LAB symbols.

// SEH-unwind epilogue funclet of an MFC __try function (parent FUN_0051ec11 /
// FUN_0051ec6a region). Restores the SEH registration and pops the frame. No
// relocations -> byte-exact.
RVA(0x0011ec59, 0x11)
__declspec(naked) void Funclet_11ec59() {
    __asm {
        mov ecx, dword ptr [ebp - 0Ch]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 10h
    }
}

// Reloc-masked externs the catch funclets reference (call targets + the EH
// continuation addresses, named by the delinker as DAT/LAB at parent code RVAs).
extern "C" void Delete_1bfe15();          // 0x1bfe15
extern "C" void AfxUnlockGlobals_1d4e45();// 0x1d4e45
extern "C" void Throw_121ca0();           // 0x121ca0 (__CxxThrowException)
extern "C" void Fn_1c0040();              // 0x1c0040
extern "C" char Cont_1bef5f; // 0x5bef5f DAT_001bef5f
extern "C" char Cont_1bc068; // 0x5bc068 DAT_001bc068
extern "C" char Cont_1bb09c; // 0x5bb09c DAT_005bb09c
extern "C" char Cont_1ca28e; // 0x5ca28e LAB_005ca28e

// catch funclet of parent FUN_005bef39's try/catch: destruct the caught local
// ([ebp-0x18]) and resume at 0x5bef5f.
RVA(0x001bef71, 0xe)
__declspec(naked) void Funclet_1bef71() {
    __asm {
        mov ecx, dword ptr [ebp - 18h]
        call Delete_1bfe15
        mov eax, offset Cont_1bef5f
        ret
    }
}

// catch funclet of parent winapi_1bbff4 (GetClassInfoA/RegisterClassA): unlock
// the MFC globals, rethrow, resume at 0x5bc068.
RVA(0x001bc087, 0x16)
__declspec(naked) void Funclet_1bc087() {
    __asm {
        push 1
        call AfxUnlockGlobals_1d4e45
        push 0
        push 0
        call Throw_121ca0
        mov eax, offset Cont_1bc068
        ret
    }
}

// catch funclet (parent in the 0x1bb000 region): dispatch a vtable slot +0x74 on
// the recovered object, store the result, destruct the local, resume at 0x5bb09c.
RVA(0x001bb075, 0x27)
__declspec(naked) void Funclet_1bb075() {
    __asm {
        call Fn_1c0040
        mov ecx, dword ptr [ebp - 14h]
        mov edx, dword ptr [eax]
        add ecx, 34h
        push ecx
        mov ecx, eax
        push dword ptr [ebp + 10h]
        call dword ptr [edx + 74h]
        mov ecx, dword ptr [ebp + 10h]
        mov dword ptr [ebp + 8], eax
        call Delete_1bfe15
        mov eax, offset Cont_1bb09c
        ret
    }
}

// catch funclet (parent in the 0x1ca200 region): two vtable dispatches (+0x90,
// +0x88) on the recovered object with the EH trylevel transitions (state 8->7),
// destruct the local, resume at 0x5ca28e.
RVA(0x001ca249, 0x45)
__declspec(naked) void Funclet_1ca249() {
    __asm {
        mov esi, dword ptr [ebp - 18h]
        mov dword ptr [ebp - 10h], esp
        push 1
        mov ecx, esi
        mov eax, dword ptr [esi]
        push dword ptr [ebp - 14h]
        call dword ptr [eax + 90h]
        mov eax, dword ptr [esi]
        push 0F102h
        push 1
        mov ecx, esi
        push dword ptr [ebp - 1Ch]
        mov byte ptr [ebp - 4], 8
        push dword ptr [ebp + 8]
        call dword ptr [eax + 88h]
        mov ecx, dword ptr [ebp - 1Ch]
        mov dword ptr [ebp - 4], 7
        call Delete_1bfe15
        mov eax, offset Cont_1ca28e
        ret
    }
}
