#ifndef GRUNTZ_GRUNTZ_CSTATE_H
#define GRUNTZ_GRUNTZ_CSTATE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/LevelArea.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CRezMgr;
class CDDSurface;
class CRezDir;

class CRezDir;
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
        b32 useOverlay
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

    i32 RetireScene(i32 pct, i32 dur, i32 lead, b32 useOverlay);

    i32 FadeLightToBlack(i32 centerX, i32 centerY, i32 durationMs, i32 leadMs);
    i32 FadeLightToBackBuffer(i32 centerX, i32 centerY, i32 durationMs, i32 leadMs);
    i32 FadeSineToBackBuffer(i32 intensityPercent, i32 durationMs, i32 leadMs);
    i32 FadeSineToBlack(i32 intensityPercent, i32 durationMs, i32 leadMs);

    void Present(i32 pct);

    CDDrawSurfaceMgr* menuRoot() {
        return m_world;
    }
    CRezMgr* ResourceArchive() {
        return static_cast<CRezMgr*>(m_resourceArchive);
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

    CRezMgr* m_resourceArchive;

    CDDrawSurfaceMgr* m_world;
    CFaderMgr* m_faderMgr;

    CDDSurface* m_ownedSurface0;

    CDDSurface* m_ownedSurface1;
    i32 m_levelIndex;
    LevelArea m_levelType;

    GameStateId m_previousStateId;

    CRezDir* m_levelResources;

    CRezDir* m_stateResources;

    CRezDir* StateResources() {
        return m_stateResources;
    }
    CRezDir* m_gruntResources;
    CRezDir* m_gameResources;
    i32 m_reserved38;
    b32 m_ready;
    b32 m_notifyLatch;

    i32 m_reserved44;
    i32 m_reserved48;

    char m_versionString[0x100];
    i32 m_reserved14c;
    Coord m_cursorPosition;
    Coord m_snapOrigin;

    CDDSurface* m_cursorSavedSurfaces[2];

    RECT m_cursorSavedRects[2];
    RECT m_cursorScreenRects[2];

    i32 m_cursorSavedSurfaceValid[2];
    i32 m_cursorBufferIndex;
};

inline CState::CState() {
    m_mgr = NULL;
    m_resourceArchive = NULL;
    m_world = NULL;
    m_levelResources = NULL;
    m_stateResources = NULL;
    m_ownedSurface0 = NULL;
    m_ownedSurface1 = NULL;
    m_reserved38 = 0;
    m_ready = false;
    m_versionString[0] = 0;
    m_previousStateId = GAMESTATE_NONE;
    m_cursorSavedSurfaces[0] = NULL;
    m_cursorSavedSurfaces[1] = NULL;
    SetRect(&m_cursorSavedRects[0], 0, 0, 0x40, 0x40);
    SetRect(&m_cursorSavedRects[1], 0, 0, 0x40, 0x40);
    SetRectEmpty(&m_cursorScreenRects[0]);
    SetRectEmpty(&m_cursorScreenRects[1]);
    m_cursorPosition.Set(0, 0);
}

#endif // GRUNTZ_GRUNTZ_CSTATE_H
