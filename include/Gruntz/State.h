#ifndef GRUNTZ_GRUNTZ_CSTATE_H
#define GRUNTZ_GRUNTZ_CSTATE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/LevelArea.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CRezArchive;
class CDDSurface;
class CRezArchiveDir;

class CRezArchiveDir;
class CFileMemBase;
class CGruntzMgr;
class CFaderMgr;
class CString;
class CMulti;

class CState {
public:
    CState();

    virtual ~CState() {
        CState::ReleaseResources();
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId);

    virtual void ReleaseResources();
    RVA(0x0008c490, 0x4)
    virtual i32 IsActive() {
        return m_ready;
    }
    RVA(0x0008c4b0, 0x6)
    virtual GameStateId Update() {
        return GAMESTATE_BASE;
    }
    RVA(0x0008c4d0, 0x6)
    virtual i32 Render() {
        return 1;
    }
    RVA(0x0008c4f0, 0x3)
    virtual i32 RestoreDisplay() {
        return 0;
    }
    virtual i32 OnPaint();

    virtual i32 InputVirtual();
    RVA(0x0008c510, 0x8)
    virtual i32 EnterState(GameStateId previousState) {
        return 1;
    }
    virtual i32 LeaveState(GameStateId nextState);

    RVA(0x0008c550, 0x5)
    virtual i32 OnChar(i32 charCode, i32 keyData) {
        return 0;
    }
    RVA(0x0008c570, 0x5)
    virtual i32 OnKeyDown(i32 virtualKey, i32 keyData) {
        return 0;
    }
    RVA(0x0008c590, 0x5)
    virtual i32 OnKeyUp(i32 virtualKey, i32 keyData) {
        return 0;
    }
    RVA(0x0008c5b0, 0x5)
    virtual i32 OnLButtonDown(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }
    RVA(0x0008c5d0, 0x5)
    virtual i32 OnLButtonUp(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }
    RVA(0x0008c5f0, 0x5)
    virtual i32 OnLButtonDblClk(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }
    RVA(0x0008c610, 0x5)
    virtual i32 OnRButtonDown(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }
    RVA(0x0008c630, 0x5)
    virtual i32 OnRButtonUp(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }
    RVA(0x0008c650, 0x5)
    virtual i32 OnRButtonDblClk(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    RVA(0x0008c670, 0x5)
    virtual i32 OnMouseMove(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    RVA(0x0008c690, 0x3)
    virtual i32 CompleteLevel() {
        return 0;
    }

    RVA(0x0008c6b0, 0x3)
    virtual i32 UnusedStateAction() {
        return 0;
    }

    virtual i32 DrawStateText(i32 x, i32 y, char* str, i32 color, i32 bkMode);

    RVA(0x0008c6d0, 0x6)
    virtual i32 PauseGame() {
        return 1;
    }
    RVA(0x0008c6f0, 0x6)
    virtual i32 ResumeGame() {
        return 1;
    }

    i32 HeaderWrite(CFileMemBase* ar);
    i32 HeaderRead(CFileMemBase* ar);

    i32 ShadeScreen(i32 pct);

    i32 LoadTitlePage(
        const char* titleName,
        i32 unused1,
        i32 unused2,
        i32 unused3,
        i32 unused4,
        i32 useOverlay
    );

    i32 DrawScreenTextImage(const char* name);
    i32 PresentTitlePage(
        const char* unusedTitleName,
        i32 unused1,
        i32 unused2,
        i32 unused3,
        i32 unused4
    );
    i32 LoadAndPresentTitlePage(
        const char* titleName,
        i32 unused1,
        i32 unused2,
        i32 unused3,
        i32 unused4
    );

    i32 RetireScene(i32 pct, i32 dur, i32 lead, i32 useOverlay);

    void Present(i32 pct);

    CDDrawSurfaceMgr* menuRoot() {
        return m_world;
    }
    CRezArchive* ResourceArchive() {
        return static_cast<CRezArchive*>(m_resourceArchive);
    }
    CGruntzMgr* owner() {
        return m_mgr;
    }
    i32 BuildAssetNamespacePrefixes(
        const CString& name,
        i32 mode,
        i32 lightGate,
        class CMulti* finishGate
    );

    CGruntzMgr* m_mgr;

    CRezArchive* m_resourceArchive;

    CDDrawSurfaceMgr* m_world;
    CFaderMgr* m_faderMgr;

    CDDSurface* m_blitSurface0;

    CDDSurface* m_blitSurface1;
    i32 m_levelIndex;
    LevelArea m_levelType;

    GameStateId m_previousStateId;

    CRezArchiveDir* m_levelResources;

    CRezArchiveDir* m_stateResources;

    CRezArchiveDir* StateResources() {
        return m_stateResources;
    }
    CRezArchiveDir* m_gruntResources;
    CRezArchiveDir* m_gameResources;
    i32 m_reserved38;
    i32 m_ready;
    i32 m_notifyLatch;

    i32 m_reserved44;
    i32 m_reserved48;

    char m_versionString[0x100];
    i32 m_reserved14c;
    i32 m_cursorX;
    i32 m_cursorY;
    i32 m_snapOriginX;
    i32 m_snapOriginY;

    CDDSurface* m_scratchSurface0;
    CDDSurface* m_scratchSurface1;

    RECT m_cursorSaveSrc0;
    RECT m_cursorSaveSrc1;
    RECT m_cursorSaveDst0;
    RECT m_cursorSaveDst1;

    i32 m_inputWarmup1;
    i32 m_inputWarmup2;
    i32 m_inputHalfSel;
};

// retail copy 0x0008c750 (emitted by gruntzmgr; pin there)
inline CState::CState() {
    m_mgr = NULL;
    m_resourceArchive = NULL;
    m_world = NULL;
    m_levelResources = NULL;
    m_stateResources = NULL;
    m_blitSurface0 = NULL;
    m_blitSurface1 = NULL;
    m_reserved38 = 0;
    m_ready = 0;
    m_versionString[0] = 0;
    m_previousStateId = GAMESTATE_NONE;
    m_scratchSurface0 = NULL;
    m_scratchSurface1 = NULL;
    m_cursorSaveSrc0.left = 0;
    m_cursorSaveSrc0.right = 0x40;
    m_cursorSaveSrc0.top = 0;
    m_cursorSaveSrc0.bottom = 0x40;
    m_cursorSaveSrc1.left = 0;
    m_cursorSaveSrc1.right = 0x40;
    m_cursorSaveSrc1.top = 0;
    m_cursorSaveSrc1.bottom = 0x40;
    m_cursorSaveDst0.left = 0;
    m_cursorSaveDst0.right = 0;
    m_cursorSaveDst0.top = 0;
    m_cursorSaveDst0.bottom = 0;
    m_cursorSaveDst1.left = 0;
    m_cursorSaveDst1.right = 0;
    m_cursorSaveDst1.top = 0;
    m_cursorSaveDst1.bottom = 0;
    m_cursorX = 0;
    m_cursorY = 0;
}

#endif // GRUNTZ_GRUNTZ_CSTATE_H
