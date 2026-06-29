// M3EhFunclets.cpp - MSVC /GX exception catch/unwind funclets and static-init
// (atexit) thunks the optimizer emits out-of-line for a parent's try/object
// cleanup. The delinker carves each at its own RVA, but no plain C++ source
// produces a standalone frameless routine that reads the parent's [ebp-N] locals
// and returns a mid-parent continuation address; they are transcribed verbatim as
// __declspec(naked) bodies (same approach as Wap32/EhFunclets.cpp). Every engine
// callee is an external symbol (reloc-masked rel32) and every continuation /
// registered address a DATA extern (reloc-masked DIR32). These are compiler/MFC
// artifacts, NOT hand-authored logic - the naked asm IS the byte form.
#include <rva.h>

extern "C" {
    void Eh_ObjDelete();        // 0x1bfe15  CObject-style delete (thiscall, ecx=this)
    void Eh_AfxGetThread();     // 0x1c0040  AfxGetThread
    void Eh_CxxThrow();         // 0x121ca0  __CxxThrowException@8 (rethrow)
    void Eh_AfxUnlockGlobals(); // 0x1d4e45  AfxUnlockGlobals
    void Dtor_5d3ef0();         // 0x5d3ef0  file-scope destructor (DIR32-masked)
    void Dtor_5d4bf3();         // 0x5d4bf3  file-scope destructor (DIR32-masked)
    void Eh_cont_5c7455();      // 0x5c7455  catch tail continuation (direct rel32 jmp)
    int __cdecl atexit(void(__cdecl*)());
    // Mid-parent continuation / registered addresses (reloc-masked DATA externs).
    char ehc_51ebfb;
    char ehc_5c9918;
    char ehc_5ba814;
    char ehc_5bb418;
    char ehc_5c7436;
    char ehc_5c7449;
    char ehc_5c7455;
    char ehc_5d3ef0;
    char ehc_5d4bf3;
    char ehc_5d4968;
    char ehc_5bd989_cont; // 0x5bd9ad
}

// 0x11ecdb - SEH-frame restore epilogue fragment (no calls / no continuation).
RVA(0x0011ecdb, 0x11)
__declspec(naked) void Funclet_11ecdb() {
    __asm {
        mov ecx, dword ptr [ebp - 0Ch]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 14h
    }
}

// 0x1bd9b0 - store result into parent's [edi+0xb8], then SEH-restore epilogue.
RVA(0x001bd9b0, 0x1d)
__declspec(naked) void Funclet_1bd9b0() {
    __asm {
        mov eax, dword ptr [ebp - 18h]
        mov ecx, dword ptr [ebp - 0Ch]
        mov dword ptr [edi + 0B8h], eax
        mov eax, dword ptr [ebp + 8]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 4
    }
}

// 0x1bb09f - copy 7 dwords ([ebp-0x40] -> [ebx+0x34]), then SEH-restore epilogue.
RVA(0x001bb09f, 0x1f)
__declspec(naked) void Funclet_1bb09f() {
    __asm {
        push 7
        mov eax, dword ptr [ebp + 8]
        lea edi, dword ptr [ebx + 34h]
        pop ecx
        lea esi, dword ptr [ebp - 40h]
        rep movsd
        mov ecx, dword ptr [ebp - 0Ch]
        pop edi
        pop esi
        mov dword ptr fs:[0], ecx
        pop ebx
        leave
        ret 14h
    }
}

// 0x11ebed - destroy a local ([ebp-0x14]), resume at 0x51ebfb.
RVA(0x0011ebed, 0xe)
__declspec(naked) void Funclet_11ebed() {
    __asm {
        mov ecx, dword ptr [ebp - 14h]
        call Eh_ObjDelete
        mov eax, offset ehc_51ebfb
        ret
    }
}

// 0x1c990a - destroy an arg local ([ebp+0xc]), resume at 0x5c9918.
RVA(0x001c990a, 0xe)
__declspec(naked) void Funclet_1c990a() {
    __asm {
        mov ecx, dword ptr [ebp + 0Ch]
        call Eh_ObjDelete
        mov eax, offset ehc_5c9918
        ret
    }
}

// 0x1ba7ff - destroy a local ([ebp-0x2c]), arm ([ebp-0x24])->+0x2c = -1, resume.
RVA(0x001ba7ff, 0x15)
__declspec(naked) void Funclet_1ba7ff() {
    __asm {
        mov ecx, dword ptr [ebp - 2Ch]
        call Eh_ObjDelete
        mov eax, dword ptr [ebp - 24h]
        or dword ptr [eax + 2Ch], 0FFFFFFFFh
        mov eax, offset ehc_5ba814
        ret
    }
}

// 0x1bb42c - rebuild a 4-arg record at [ebp-0x3c], dispatch it through the
// current thread (AfxGetThread vtbl +0x74), destroy [ebp-0x20], resume.
RVA(0x001bb42c, 0x3c)
__declspec(naked) void Funclet_1bb42c() {
    __asm {
        mov eax, dword ptr [ebp + 8]
        mov dword ptr [ebp - 3Ch], eax
        mov eax, dword ptr [ebp + 0Ch]
        mov dword ptr [ebp - 38h], eax
        mov eax, dword ptr [ebp + 10h]
        mov dword ptr [ebp - 34h], eax
        mov eax, dword ptr [ebp + 14h]
        mov dword ptr [ebp - 30h], eax
        call Eh_AfxGetThread
        mov edx, dword ptr [eax]
        lea ecx, dword ptr [ebp - 3Ch]
        push ecx
        mov ecx, eax
        push dword ptr [ebp - 20h]
        call dword ptr [edx + 74h]
        mov ecx, dword ptr [ebp - 20h]
        mov dword ptr [ebp - 14h], eax
        call Eh_ObjDelete
        mov eax, offset ehc_5bb418
        ret
    }
}

// 0x1c741c - catch funclet: if the caught object ([ebp-0x20]) is kind 3 destroy
// it and (unless [ebp-0x18]) resume at 0x5c7436, else rethrow; the kind-3/flag
// path tail resumes at 0x5c7449.
RVA(0x001c741c, 0x29)
__declspec(naked) void Funclet_1c741c() {
    __asm {
        mov ecx, dword ptr [ebp - 20h]
        cmp dword ptr [ecx + 8], 3
        jne L_rethrow
        call Eh_ObjDelete
        cmp dword ptr [ebp - 18h], 0
        jne L_tail
        mov eax, offset ehc_5c7436
        ret
        xor eax, eax
        jmp Eh_cont_5c7455
    L_rethrow:
        push 0
        push 0
        call Eh_CxxThrow
    L_tail:
        mov eax, offset ehc_5c7449
        ret
    }
}

// 0x1d3ee4 / 0x1d4be7 - compiler atexit registration thunks for two file-scope
// destructors. Written as the real atexit() call: push-imm(dtor, DIR32-masked) +
// call atexit(rel32-masked) are byte-exact.
// @early-stop
// cdecl-cleanup idiom wall (85%): the compiler-emitted static-init thunk pops the
// 1-dword arg with `pop ecx` (1 byte); a source-level atexit() call cleans it with
// `add esp,4` (3 bytes) - the only code difference (plus trailing alignment nops).
// Reaching `pop ecx` needs the compiler to *synthesize* the registration thunk for
// a static object dtor, which cannot be pinned to this RVA. Verified llvm-objdump -dr. for two file-scope
// destructors (0x5d3ef0 / 0x5d4bf3). clang's MS-asm parser crashes on
// `push offset`, so these are written as the real atexit() call they compile from
// (push imm dtor; call atexit; pop ecx; ret); both the dtor address (DIR32) and
// the atexit call (rel32) are reloc-masked.
RVA(0x001d3ee4, 0xc)
void Funclet_1d3ee4() {
    atexit(&Dtor_5d3ef0);
}

RVA(0x001d4be7, 0xc)
void Funclet_1d4be7() {
    atexit(&Dtor_5d4bf3);
}

// 0x1d4952 - unlock globals (lock 0x10) then rethrow; resume at 0x5d4968.
RVA(0x001d4952, 0x16)
__declspec(naked) void Funclet_1d4952() {
    __asm {
        push 10h
        call Eh_AfxUnlockGlobals
        push 0
        push 0
        call Eh_CxxThrow
        mov eax, offset ehc_5d4968
        ret
    }
}

// 0x1bd989 - one-instruction handler stub: load the continuation 0x5bd9ad.
RVA(0x001bd989, 0x6)
__declspec(naked) void Funclet_1bd989() {
    __asm {
        mov eax, offset ehc_5bd989_cont
        ret
    }
}
