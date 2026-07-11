// ScreenRegionMgr.cpp - the screen/video-region manager (Open @0x0fe460 / Reset
// @0x0fe600), split out of GruntzMgr.cpp: these two methods are a separate retail
// object far from the CGruntzMgr block, interleaved with the CSBI_RectOnly .text
// (SBI_RectOnly.cpp) at 0x0fe4xx. Same "eh" flags; byte-neutral TU cut.
//
// @identity-TODO: reached as a sub-object [owner+0x2dc] (xref: CGruntzMgr::SetVideoMode,
// CPlay::OnKeyCommand, CSBI_RectOnly::LoadBattlezItemConfig all call it on `this+0x2dc`);
// an embedded sub-object whose concrete RTTI name is not yet recovered. Field NAMES are
// placeholders (only offsets + code bytes are load-bearing).
#include <Mfc.h>                 // RECT / Win32 SetRect (via the afx-controlled windows.h)
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_curState / ReportError)
#include <rva.h>

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). DATA-bound in
// GruntzMgr.cpp (many CGruntzMgr methods use it); declared-only here.
extern "C" CGameRegistry* g_gameReg;

void __stdcall Prep_12fd(i32 mode); // 0x12fd (free __stdcall region-prep helper)
// The live game-state sub-call (g_gameReg->m_curState->Sub3d55, 0x3d55) both
// methods make; reloc-masked thiscall (the state's concrete method not recovered).
struct ScreenCurState {
    // Sub3d55 @0xfe460 IS ScreenRegionMgr::Open; cast at each call.
};
struct ScreenRegionMgr {
    i32 m_0; // +0x00  state (1 == open, 2 == reset)
    char m_pad4[0x10 - 4];
    RECT m_10; // +0x10  region rect
    char m_pad20[0x10c - 0x20];
    i32 m_10c; // +0x10c  activate arg
    char m_pad110[0x548 - 0x110];
    i32 m_548;                   // +0x548  busy/one-shot flag
    i32 Open();                  // 0x0fe460
    i32 Reset();                 // 0x0fe600
    void Sub194c(i32 v);         // 0x194c  thiscall (resize/layout)
    i32 Validate();              // 0x3a08  thiscall
    void Activate(i32 a, i32 n); // 0x1d61  thiscall
};
SIZE_UNKNOWN(ScreenCurState);
SIZE_UNKNOWN(ScreenRegionMgr);
RVA(0x000fe460, 0x83)
i32 ScreenRegionMgr::Open() {
    if (m_548 == 0 && m_0 != 1) {
        Prep_12fd(1);
        SetRect(&m_10, 0, 0, 0xa0, 0x1e0);
        Sub194c(1);
        ((ScreenRegionMgr*)g_gameReg->m_curState)->Open();
        if (!Validate()) {
            g_gameReg->ReportError(0x80e4, 0x448);
            return 0;
        }
        Activate(m_10c, 3);
    }
    return 1;
}
RVA(0x000fe600, 0x49)
i32 ScreenRegionMgr::Reset() {
    if (m_548 == 0 && m_0 != 2) {
        Prep_12fd(1);
        SetRect(&m_10, -1, -1, -1, -1);
        Sub194c(2);
        ((ScreenRegionMgr*)g_gameReg->m_curState)->Open();
    }
    return 1;
}
