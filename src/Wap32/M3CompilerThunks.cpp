// M3CompilerThunks.cpp - four compiler/MFC-emitted thunks with no plain-C++ source
// form (a C++ object destructor with a /GX SEH frame, a global-Font dynamic
// initializer, and two MFC global-object call thunks). Transcribed verbatim as
// __declspec(naked); every callee/global/scope-table is an external symbol
// (reloc-masked rel32/DIR32). Same sanctioned approach as Wap32/EhFunclets.cpp.
#include <rva.h>

extern "C" {
void Font_ctor_179700(); // 0x179700 Font::Font (tail-called by the initializer)
void g_smallFont_64eb00(); // 0x64eb00 the global small Font (address-of)
void g_obj_652e40();     // 0x652e40 MFC global object
void Wnd_1baf15();       // 0x1baf15 method invoked on g_652e40
}

// 0x1156b0 - dynamic initializer for the global small Font: construct it (tail
// call to Font::Font with ecx = &g_smallFont).
RVA(0x001156b0, 0xa)
__declspec(naked) void Init_smallFont() {
    __asm {
        mov ecx, offset g_smallFont_64eb00
        jmp Font_ctor_179700
    }
}

// 0x1bade1 - invoke a method on an MFC global object with arg 0.
RVA(0x001bade1, 0xd)
__declspec(naked) void Thunk_1bade1() {
    __asm {
        push 0
        mov ecx, offset g_obj_652e40
        call Wnd_1baf15
        ret
    }
}

// 0x1d3330 - invoke a method on an MFC global object passing a fixed code address;
// real C++ so the compiler emits the `push offset` (push imm32) that retail uses.
struct CObj653070 {
    void* Mthd_1d485c(void* arg); // 0x1d485c (thiscall, external)
};
extern CObj653070 g_obj653070; // 0x653070
extern "C" void str_5d2fe9();  // 0x5d2fe9 pushed code address

RVA(0x001d3330, 0x10)
void Thunk_1d3330() {
    g_obj653070.Mthd_1d485c((void*)&str_5d2fe9);
}
