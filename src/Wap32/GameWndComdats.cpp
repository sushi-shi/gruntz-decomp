#include <rva.h>

#include <Mfc.h>

#include <Wap32/Wap32.h>

// @identity-TODO ?1CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00094c10, 0x16, ??1CGameWnd@@UAE@XZ)

// @identity-TODO OnKeyUp@CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x00094c80, 0x5)
i32 CGameWnd::OnKeyUp(WPARAM, LPARAM) {
    return 0;
}
i32 CGameWnd::OnSysKeyDown(WPARAM, LPARAM) {
    return 0;
}
i32 CGameWnd::OnLButtonDown(WPARAM, i32, i32) {
    return 0;
}
// @identity-TODO OnRButtonDown@CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x00094cc0, 0x5)
i32 CGameWnd::OnRButtonDown(WPARAM, i32, i32) {
    return 0;
}
// @identity-TODO OnLButtonUp@CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x00094ce0, 0x5)
i32 CGameWnd::OnLButtonUp(WPARAM, i32, i32) {
    return 0;
}
// @identity-TODO OnRButtonUp@CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x00094d00, 0x5)
i32 CGameWnd::OnRButtonUp(WPARAM, i32, i32) {
    return 0;
}
i32 CGameWnd::OnMouseMove(WPARAM, i32, i32) {
    return 0;
}
// @identity-TODO OnLButtonDblClk@CGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA(0x00094d40, 0x5)
i32 CGameWnd::OnLButtonDblClk(WPARAM, i32, i32) {
    return 0;
}
i32 CGameWnd::OnRButtonDblClk(WPARAM, i32, i32) {
    return 0;
}
// @identity-TODO ?_GCGameWnd - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (10 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00094d80, 0x2f, ??_GCGameWnd@@UAEPAXI@Z)
