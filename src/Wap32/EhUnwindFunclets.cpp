// EhUnwindFunclets.cpp - compiler-emitted C++ EH funclets / unwind fragments and
// CRT EH glue that Ghidra carved as standalone "functions".  Each is a piece of a
// larger parent function's /GX state machine (a destructor-unwind funclet, a catch
// block, an SEH-frame teardown tail, or an atexit registration stub) and has no
// standalone C++ source form, so it is reconstructed as a naked __asm body whose
// bytes are literal.  The `call`/`OFFSET` operands resolve to external no-body
// symbols, so their relocations are masked by objdiff (the code bytes match).
//
// Owners are unidentified (anonymous parents); these are grouped here rather than
// homed to a class - names are matching-neutral.  See docs/patterns/.
#include <rva.h>

// --- external engine callees / continuation labels (reloc-masked) -------------
extern "C" void FUN_524df0();           // 0x124df0 - CRT terminate/EH helper
extern "C" void Delete_1bfe15();        // 0x1bfe15 - destructor helper (thiscall via ecx)
extern "C" void AfxSetNewHandler_1b9b32(); // 0x1b9b32
extern "C" int  atexit_11f490();        // 0x11f490 - atexit
extern int Cont_11ecd6;                 // 0x51ecd6 continuation
extern int Cont_1ca279;                 // 0x5ca279 continuation
extern int Cont_1ca0e1;                 // 0x5ca0e1 continuation
extern "C" void Lbl_1d3190();           // 0x5d3190 static-dtor thunk (push OFFSET needs a fn sym)
extern "C" void Lbl_1d427d();           // 0x5d427d static-dtor thunk
extern int Dat_2160cc;                  // 0x6160cc

// 0x124f0e - CRT EH wrapper: tail-call-suppressed call to the terminate helper.
RVA(0x00124f0e, 0x6)
__declspec(naked) void Unmatched_124f0e() {
    __asm {
        call FUN_524df0
        ret
    }
}

// 0x11f760 - C++ exception filter: if the dispatched ExceptionRecord carries the
// MSC magic (0xe06d7363) run the terminate helper, then return 0.
RVA(0x0011f760, 0x16)
int Unmatched_11f760(unsigned** p) {
    if (**p == 0xe06d7363u) {
        FUN_524df0();
    }
    return 0;
}

// 0x11ecc8 - destructor-unwind funclet (this from [ebp+8]).
RVA(0x0011ecc8, 0xe)
__declspec(naked) void Unmatched_11ecc8() {
    __asm {
        mov ecx, dword ptr [ebp+8]
        call Delete_1bfe15
        mov eax, OFFSET Cont_11ecd6
        ret
    }
}

// 0x1ca2c3 - destructor-unwind funclet (this from [ebp-0x18]).
RVA(0x001ca2c3, 0xe)
__declspec(naked) void Unmatched_1ca2c3() {
    __asm {
        mov ecx, dword ptr [ebp-0x18]
        call Delete_1bfe15
        mov eax, OFFSET Cont_1ca279
        ret
    }
}

// 0x11ec00 - /GX frame teardown tail (restore SEH reg, pop saves, ret 0x10).
RVA(0x0011ec00, 0x11)
__declspec(naked) void Unmatched_11ec00() {
    __asm {
        mov ecx, dword ptr [ebp-0xc]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 0x10
    }
}

// 0x1c744f - /GX frame teardown tail with trailing zero-store + result in edi.
RVA(0x001c744f, 0x17)
__declspec(naked) void Unmatched_1c744f() {
    __asm {
        and byte ptr [esi+edi], 0
        mov eax, edi
        mov ecx, dword ptr [ebp-0xc]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 8
    }
}

// 0x1c11a5 - catch/finally funclet: restore new-handler, store result fields,
// then /GX teardown (ret 4).
RVA(0x001c11a5, 0x31)
__declspec(naked) void Unmatched_1c11a5() {
    __asm {
        push dword ptr [ebp-0x18]
        or dword ptr [ebp-4], 0xffffffff
        call AfxSetNewHandler_1b9b32
        mov eax, dword ptr [esi+0x3c]
        add eax, dword ptr [ebp-0x14]
        mov dword ptr [eax], edi
        cmp dword ptr [esi+0x40], 2
        jne skip
        mov dword ptr [eax+4], edi
    skip:
        mov eax, dword ptr [ebp-0x14]
        mov ecx, dword ptr [ebp-0xc]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 4
    }
}

// 0x1ca095 - catch funclet: drive two virtual calls + a third, walk the /GX state
// byte, then run the destructor helper and return the continuation address.
RVA(0x001ca095, 0x4c)
__declspec(naked) void Unmatched_1ca095() {
    __asm {
        mov esi, dword ptr [ebp-0x18]
        mov dword ptr [ebp-0x10], esp
        push 1
        mov ecx, esi
        mov eax, dword ptr [esi]
        push dword ptr [ebp-0x14]
        call dword ptr [eax+0x90]
        mov eax, dword ptr [esi]
        mov ecx, esi
        call dword ptr [eax+0x74]
        mov eax, dword ptr [esi]
        push 0xf101
        push 0
        mov ecx, esi
        push dword ptr [ebp-0x1c]
        mov byte ptr [ebp-4], 8
        push dword ptr [ebp+8]
        call dword ptr [eax+0x88]
        mov ecx, dword ptr [ebp-0x1c]
        mov dword ptr [ebp-4], 7
        call Delete_1bfe15
        mov eax, OFFSET Cont_1ca0e1
        ret
    }
}

// 0x1d3184 - file-scope static destructor registration (atexit thunk).
RVA(0x001d3184, 0xc)
__declspec(naked) void Unmatched_1d3184() {
    __asm {
        push OFFSET Lbl_1d3190
        call atexit_11f490
        pop ecx
        ret
    }
}

// 0x1d4271 - file-scope static destructor registration (atexit thunk).
RVA(0x001d4271, 0xc)
__declspec(naked) void Unmatched_1d4271() {
    __asm {
        push OFFSET Lbl_1d427d
        call atexit_11f490
        pop ecx
        ret
    }
}

// 0x11f23d - alternate entry into a shared CRT EH tail (seeds ebx then a short
// jmp into the merged body at 0x11f250).  The 2-byte short jmp displacement is a
// literal (its target is outside this carved range).
RVA(0x0011f23d, 0x9)
__declspec(naked) void Unmatched_11f23d() {
    __asm {
        push ebx
        push ecx
        mov ebx, OFFSET Dat_2160cc
        _emit 0xeb
        _emit 0x0a
    }
}
