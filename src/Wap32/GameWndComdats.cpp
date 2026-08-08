#include <rva.h>

#include <Mfc.h>

#include <Wap32/Wap32.h>

// The ten CGameWnd virtuals retail laid down as one 0x20-strided run at
// 0x94c40..0x94d64, disjoint from the gamewnd TU's own band at 0x13d3xx.
// Recovered from the retail vtable slots (gruntz.audit.vtable_slot_labels):
// every slot address is a bare `xor eax,eax / ret <argbytes>`.

RVA(0x00094c40, 0x5)
i32 CGameWnd::PreDispatchMessage(UINT, WPARAM, LPARAM) {
    return 0;
}

RVA(0x00094c60, 0x5)
i32 CGameWnd::HandleWindowCommand(i32, i32, i32) {
    return 0;
}

RVA(0x00094c80, 0x5)
i32 CGameWnd::OnKeyUp(WPARAM, LPARAM) {
    return 0;
}

RVA(0x00094ca0, 0x5)
i32 CGameWnd::OnLButtonDown(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094cc0, 0x5)
i32 CGameWnd::OnRButtonDown(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094ce0, 0x5)
i32 CGameWnd::OnLButtonUp(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094d00, 0x5)
i32 CGameWnd::OnRButtonUp(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094d20, 0x5)
i32 CGameWnd::OnMouseMove(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094d40, 0x5)
i32 CGameWnd::OnLButtonDblClk(WPARAM, i32, i32) {
    return 0;
}

RVA(0x00094d60, 0x5)
i32 CGameWnd::OnRButtonDblClk(WPARAM, i32, i32) {
    return 0;
}
