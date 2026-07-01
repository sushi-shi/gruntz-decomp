// ComHelperThunks.cpp - small leaf glue from the engine's COM/registry-helper
// module (the 0x1bf*-0x1d5* run): a vptr-stamping base ctor, a detach getter,
// three static-destructor atexit-registration thunks, and two one-line forwarders
// onto file-scope engine singletons. All callees/globals are external symbols
// (reloc-masked rel32/DIR32); only the instruction selection is load-bearing.
#include <rva.h>

extern "C" int atexit(void (*func)(void));

// The static destructors the three atexit thunks register (external, reloc-masked).
extern "C" void EhStaticDtor_5ce97f();
extern "C" void EhStaticDtor_5d4245();
extern "C" void EhStaticDtor_5d4c22();

// The scalar-deleting-destructor vtable the base ctor stamps (VA 0x5ec26c).
DATA(0x001ec26c)
extern void* g_vtbl_5ec26c;

// File-scope engine singletons the forwarders dispatch onto.
struct ComSingleton2f00 {
    void Activate(int on); // 0x1baf15
};
DATA(0x00252f00)
extern ComSingleton2f00 g_com652f00; // VA 0x652f00

struct ComSingleton3210 {
    void Pump(); // 0x1d4c7e
};
DATA(0x00253210)
extern ComSingleton3210 g_com653210; // VA 0x653210

// A base subobject whose ctor only stamps the vtable pointer.
struct ComStamp {
    void Ctor(); // 0x1d4722
};

// ---------------------------------------------------------------------------
// 0x1c0a41 - return *this(+0) and clear it. Retail zeroes via `and [ecx],0`
// (3-byte sign-extended-imm8 AND); MSVC5 lowers any C++ `=0`/`&=0` to a 6-byte
// `mov [ecx],0`, so the AND form is reconstructed verbatim.
RVA(0x001c0a41, 0x6)
__declspec(naked) void ComDetach_1c0a41() {
    __asm {
        mov eax, dword ptr [ecx]
        and dword ptr [ecx], 0
        ret
    }
}

// ---------------------------------------------------------------------------
// 0x1d4722 - stamp the base vtable pointer.
// ---------------------------------------------------------------------------
RVA(0x001d4722, 0x7)
void ComStamp::Ctor() {
    *(void**)this = &g_vtbl_5ec26c;
}

// ---------------------------------------------------------------------------
// 0x1d4c0c - forward to the session-pump singleton.
// ---------------------------------------------------------------------------
RVA(0x001d4c0c, 0xa)
void ComPump3210() {
    g_com653210.Pump();
}

// ---------------------------------------------------------------------------
// 0x1bae1f - activate the COM singleton.
// ---------------------------------------------------------------------------
RVA(0x001bae1f, 0xd)
void ComActivate2f00() {
    g_com652f00.Activate(1);
}

// ---------------------------------------------------------------------------
// 0x1ce973 / 0x1d4239 / 0x1d4c16 - register a static object's destructor. These
// are CRT-emitted registration stubs whose 1-dword cdecl cleanup is `pop ecx`;
// MSVC5 /O2 lowers a C++ `atexit(fn)` to `add esp,4`, so they are verbatim.
RVA(0x001ce973, 0xc)
__declspec(naked) void ComAtexit_5ce97f() {
    __asm {
        push offset EhStaticDtor_5ce97f
        call atexit
        pop ecx
        ret
    }
}

RVA(0x001d4239, 0xc)
__declspec(naked) void ComAtexit_5d4245() {
    __asm {
        push offset EhStaticDtor_5d4245
        call atexit
        pop ecx
        ret
    }
}

RVA(0x001d4c16, 0xc)
__declspec(naked) void ComAtexit_5d4c22() {
    __asm {
        push offset EhStaticDtor_5d4c22
        call atexit
        pop ecx
        ret
    }
}

// Class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(ComSingleton2f00); // COM singleton forward view
SIZE_UNKNOWN(ComSingleton3210); // session-pump singleton forward view
SIZE_UNKNOWN(ComStamp);         // vptr-stamp base subobject view
