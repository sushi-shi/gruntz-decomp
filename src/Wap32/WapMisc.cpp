// WapMisc.cpp - assorted tiny engine leaves (anonymous owners).
//
//  0x11d100 - vtable-stamp leaf: *this = &vtbl (a ctor/teardown that only sets vptr).
//  0x1c7e6d - CView-derived destructor: reset vptr then tail-jmp the base ~CView.
//  0x1155b0 - forward a no-arg call to a method on the g_largeFont global (tail-jmp).
//  0x1bae5d - forward (-1) to a method on a global CWnd-like object.
//  0xc76d0  - forward (0x7d0,0x7da) to a method on a global object.
#include <rva.h>

// --- 0x11d100 : vtable-stamp leaf --------------------------------------------
extern void* const Vtbl_5ec26c;
struct CStamp11d100 {
    void* vptr;
    void Stamp();
};
RVA(0x0011d100, 0x7)
void CStamp11d100::Stamp() {
    vptr = (void*)&Vtbl_5ec26c;
}

// --- 0x1c7e6d : CView-derived dtor (vptr reset + tail-jmp base ~CView) --------
extern void* const Vtbl_5ed854;
extern "C" void DtorCView_1c8e3a(); // 0x1c8e3a ~CView
RVA(0x001c7e6d, 0xb)
__declspec(naked) void Dtor_1c7e6d() {
    __asm {
        mov dword ptr [ecx], OFFSET Vtbl_5ed854
        jmp DtorCView_1c8e3a
    }
}

// --- 0x1155b0 : forward to a g_largeFont method ------------------------------
struct Font {
    void Method_179700(); // 0x179700
};
extern Font g_largeFont; // ?g_largeFont@@3VFont@@A (0x64eac0)
RVA(0x001155b0, 0xa)
void Unmatched_1155b0() {
    g_largeFont.Method_179700();
}

// --- 0x1bae5d : forward (-1) to a global CWnd-like method --------------------
struct WndLike {
    void Method_1baf15(int); // 0x1baf15
};
extern WndLike g_652e80;
RVA(0x001bae5d, 0xd)
void Unmatched_1bae5d() {
    g_652e80.Method_1baf15(-1);
}

// --- 0xc76d0 : forward (0x7d0,0x7da) to a global object's method -------------
struct Obj64bf00 {
    void Method_8710(int, int); // 0x8710
};
extern Obj64bf00 g_64bf00;
RVA(0x000c76d0, 0x15)
void Unmatched_c76d0() {
    g_64bf00.Method_8710(0x7d0, 0x7da);
}
