#include <rva.h>

#include <EmptyString.h>
#include <Gruntz/GruntzCommandId.h>
#include <Wap32/GameApp.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

VTBL(CGameMgr, 0x001e9b8c);
RVA_COMPGEN(0x00080cf0, 0x12, ??1CGameApp@@UAE@XZ)

// @identity-TODO InitDefault@CGameApp - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (23 fns) came from the static library. It belongs to another compiland.
RVA(0x00080d20, 0x24)
i32 CGameApp::InitDefault(HINSTANCE hInstance, char* szName) {
    return Init(
        hInstance,
        szName,
        szName,
        g_emptyString,
        0,
        static_cast<i32>(0x80000000),
        static_cast<i32>(0x80000000)
    );
}

// @identity-TODO HasWindowAndManager@CGameApp - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (23 fns) came from the static library. It belongs to another compiland.
RVA(0x00080d60, 0x18)
i32 CGameApp::HasWindowAndManager() {
    return m_gameWnd != 0 && m_gameMgr != 0;
}

// @identity-TODO HandleCommand@CGameApp - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (23 fns) came from the static library. It belongs to another compiland.
RVA(0x00080d90, 0x5)
i32 CGameApp::HandleCommand(i32, GruntzCommandId, i32) {
    return 0;
}

// @identity-TODO ?_GCGameApp - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (23 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x00080dd0, 0x32, ??_GCGameApp@@UAEPAXI@Z)
