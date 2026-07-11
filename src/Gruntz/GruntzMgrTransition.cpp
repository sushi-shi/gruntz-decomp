// GruntzMgrTransition.cpp - CDemo::Vslot15 (0x3c030), the demo-state WM_COMMAND re-post.
// CGruntzMgr::TransitionState (0x8b960) was re-homed to src/Gruntz/GruntzMgr.cpp (waveP):
// its retail birth position is inside that TU's 0x8b8c0 interval (its /GX state factory).
// This unit retains only the Vslot15 leaf (the 0x3c030 stray, out of scope this batch).
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Demo.h>
#include <rva.h>

// The cached PostMessageA fn-ptr (bare 0x6c44c8; DATA home in Play.cpp). Reloc-masked.
extern i32(WINAPI* g_pPostMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x6c44c8

// CDemo::Vslot15 (slot 21, 0x3c030): post WM_COMMAND 0x8027 to the owner HWND (the
// CState/CWorld back-ptr chain m_4w()->m_4->m_4). Returns 1.
RVA(0x0003c030, 0x22)
i32 CDemo::Vslot15() {
    g_pPostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8027, 0);
    return 1;
}

SIZE_UNKNOWN(CDemo);
