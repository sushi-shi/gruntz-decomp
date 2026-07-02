// WapMisc.cpp - assorted tiny engine leaves.
//
//  0x11d100 - a CNoTrackObject base-subobject vptr stamp (*this = &??_7CNoTrackObject);
//             transitional stamp - the class lives in ComHelperThunks.cpp.
//  0x1155b0 - construct the g_largeFont global (tail-jmp to ??0Font).
//  0x1bae5d - forward (-1) to a CWnd method on a global framework object.
//  0xc76d0  - construct g_64bf00 (CZDArrayDerived::Construct(0x7d0,0x7da)).
#include <rva.h>
#include <Ints.h>

// --- 0x11d100 : CNoTrackObject vptr stamp ------------------------------------
// Stamps the ??_7CNoTrackObject vtable (VA 0x5ec26c = the vtable ComHelperThunks.cpp
// models as CNoTrackObject). Kept as a bare stamp leaf: making it a real CNoTrackObject
// ctor would add a `mov eax,ecx` (return-this) the 7-byte retail body does not carry
// (the vtable-realization boundary - matcher.md). CNoTrackObject itself lives in a
// different TU, so the stamp is modeled locally against the same vtable symbol.
extern void* const Vtbl_5ec26c; // ??_7CNoTrackObject@@6B@
struct CNoTrackObjectStamp {
    void* vptr;
    void Stamp();
};
RVA(0x0011d100, 0x7)
void CNoTrackObjectStamp::Stamp() {
    vptr = (void*)&Vtbl_5ec26c;
}

// --- 0x1155b0 : construct the g_largeFont global -----------------------------
// Tail-jmp to ??0Font (0x179700 == the Font default ctor); a dynamic-initializer
// thunk that constructs the global font. Font is the real engine font class.
struct Font {
    void Construct(); // 0x179700 (??0Font@@QAE@XZ)
};
extern Font g_largeFont; // ?g_largeFont@@3VFont@@A (0x64eac0)
RVA(0x001155b0, 0xa)
void Unmatched_1155b0() {
    g_largeFont.Construct();
}

// --- 0x1bae5d : forward (-1) to a CWnd method on a global object -------------
// g_652e80 is an MFC framework global; 0x1baf15 is a CWnd method (Ghidra: CWnd)
// taking an int, shared with ComHelperThunks' g_com652f00 forwarder. The exact
// CWnd-family class of the global is not recoverable from this one call site.
struct CWndFwd {
    void Method_1baf15(int); // 0x1baf15 (CWnd method)
};
extern CWndFwd g_652e80;
RVA(0x001bae5d, 0xd)
void Unmatched_1bae5d() {
    g_652e80.Method_1baf15(-1);
}

// --- 0xc76d0 : construct the g_64bf00 global (CZDArrayDerived) ----------------
// 0x8710 == CZDArrayDerived::Construct (the 2D typed-array build in ZDArrayDerived.cpp);
// this leaf builds the global with (0x7d0, 0x7da). Local decl matches that class.
struct CZDArrayDerived {
    CZDArrayDerived* Construct(i32 lo, i32 hi); // 0x8710
};
extern CZDArrayDerived g_64bf00;
RVA(0x000c76d0, 0x15)
void Unmatched_c76d0() {
    g_64bf00.Construct(0x7d0, 0x7da);
}

SIZE_UNKNOWN(CNoTrackObjectStamp); // CNoTrackObject vptr-stamp leaf
SIZE_UNKNOWN(CWndFwd);             // MFC CWnd-family global forward (class not recoverable)
