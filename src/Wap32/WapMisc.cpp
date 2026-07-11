// WapMisc.cpp - assorted tiny engine leaves.
//
//  0x11d100 - a CNoTrackObject base-subobject vptr stamp (*this = &??_7CNoTrackObject);
//             transitional stamp - the class lives in ComHelperThunks.cpp.
//  0x1155b0 - construct the g_largeFont global (tail-jmp to ??0Font).
//  0x1bae5d - forward (-1) to a CWnd method on a global framework object.
#include <rva.h>
#include <Ints.h>

#include <Font/Font.h> // real Font (the g_largeFont global)
#include <Wap32/ZVec.h>

// --- 0x11d100 : CNoTrackObject vptr stamp ------------------------------------
// Stamps the ??_7CNoTrackObject vtable (VA 0x5ec26c = the vtable ComHelperThunks.cpp
// models as CNoTrackObject). Kept as a bare stamp leaf: making it a real CNoTrackObject
// ctor would add a `mov eax,ecx` (return-this) the 7-byte retail body does not carry
// (the vtable-realization boundary - matcher.md). CNoTrackObject itself lives in a
// different TU, so the stamp is modeled locally against the same vtable symbol.
extern void* const Vtbl_5ec26c; // ??_7CNoTrackObject@@6B@
struct CNoTrackObjectStamp {
    void* vptr;
    void Stamp(); // 0x11d100
};
// Out-of-line so it emits: an inline member of a leaf with no in-TU caller folds
// away and never produces a standalone body at 0x11d100.
RVA(0x0011d100, 0x7)
void CNoTrackObjectStamp::Stamp() {
    vptr = (void*)&Vtbl_5ec26c;
}

// --- 0x1155b0 : construct the g_largeFont global -----------------------------
// A dynamic-initializer thunk that constructs the global font in place via the
// explicit-ctor-call tail-jmp (mov ecx,&g_largeFont; jmp ??0Font@@QAE@XZ 0x179700 -
// no placement-new null-guard). Font is the real engine font class (<Font/Font.h>).
extern Font g_largeFont; // ?g_largeFont@@3VFont@@A (0x64eac0)
RVA(0x001155b0, 0xa)
void Unmatched_1155b0() {
    g_largeFont.Font::Font();
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

// (The 0xc76d0 "Unmatched_c76d0" that was parked here is really
//  CDroppedObjectShadow::InitActReg - src/Gruntz/DroppedObject.cpp, wave2-H.)

// --- 0x1bf577 : free the owned module handle if present ----------------------
// A __thiscall leaf on a placeholder host that owns an HMODULE at +0x00. Re-homed
// from src/Stub/ApiMiscHelpers.cpp.
struct LibHost_1bf577 {
    HMODULE m_0; // +0x00
    void Run();
};
SIZE_UNKNOWN(LibHost_1bf577);
RVA(0x001bf577, 0xe)
void LibHost_1bf577::Run() {
    if (m_0) {
        FreeLibrary(m_0);
    }
}

// --- 0x1c09de : free the owned global handle if present ----------------------
// A __thiscall leaf on a placeholder host that owns an HGLOBAL at +0x00. Re-homed
// from src/Stub/ApiMiscHelpers.cpp (its RVA neighborhood).
// @identity-TODO: owner class genuinely unrecovered - a minimal __thiscall placeholder.
struct GlobalHandleOwner {
    HGLOBAL m_handle; // +0x00
    void FreeHandle();
};
SIZE_UNKNOWN(GlobalHandleOwner);
RVA(0x001c09de, 0xe)
void GlobalHandleOwner::FreeHandle() {
    if (m_handle) {
        GlobalFree(m_handle);
    }
}

SIZE_UNKNOWN(CNoTrackObjectStamp); // CNoTrackObject vptr-stamp leaf
SIZE_UNKNOWN(CWndFwd);             // MFC CWnd-family global forward (class not recoverable)
