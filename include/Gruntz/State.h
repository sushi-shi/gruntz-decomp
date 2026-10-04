#ifndef GRUNTZ_GRUNTZ_CSTATE_H
#define GRUNTZ_GRUNTZ_CSTATE_H

#include <string>
#include <Runtime/FadePlayback.h>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/LevelArea.h>
#include <Ints.h>
#include <RectMacros.h>

class CDDrawSurfaceMgr;
class CRezMgr;
class CDDSurface;
class CRezDir;

class CRezDir;
class CFileMemBase;
class CGruntzMgr;
class CFaderMgr;

class CMulti;

class CState {
public:
    CState();

    virtual ~CState() {
        CState::ReleaseResources();
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId);

    virtual void ReleaseResources();

    virtual i32 IsActive() {
        return m_ready;
    }

    virtual GameStateId Update() {
        return GAMESTATE_BASE;
    }

    virtual i32 Render() {
        return 1;
    }

    virtual i32 RestoreDisplay() {
        return 0;
    }
    virtual i32 OnPaint();

    virtual i32 InputVirtual();

    virtual i32 EnterState(GameStateId previousState) {
        return 1;
    }
    virtual i32 LeaveState(GameStateId nextState);

    virtual i32 OnChar(i32 charCode, i32 keyData) {
        return 0;
    }

    virtual i32 OnKeyDown(i32 virtualKey, i32 keyData) {
        return 0;
    }

    virtual i32 OnKeyUp(i32 virtualKey, i32 keyData) {
        return 0;
    }

    virtual i32 OnLButtonDown(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnLButtonUp(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnLButtonDblClk(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonDown(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonUp(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnRButtonDblClk(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 OnMouseMove(i32 keyFlags, i32 x, i32 y) {
        return 0;
    }

    virtual i32 CompleteLevel() {
        return 0;
    }

    virtual i32 UnusedStateAction() {
        return 0;
    }

    virtual i32 DrawStateText(i32 x, i32 y, char* str, i32 color, i32 bkMode);

    virtual i32 PauseGame() {
        return 1;
    }

    virtual i32 ResumeGame() {
        return 1;
    }

    i32 HeaderWrite(CFileMemBase* ar);
    i32 HeaderRead(CFileMemBase* ar);

    i32 ShadeScreen(i32 pct);

    i32 LoadTitlePage(
        const std::string& titleName,
        i32 unused1,
        i32 unused2,
        i32 unused3,
        i32 unused4,
        b32 useOverlay
    );

    i32 DrawScreenTextImage(const std::string& name);
    i32 PresentTitlePage();
    i32 LoadAndPresentTitlePage(
        const std::string& titleName,
        i32 unused1,
        i32 unused2,
        i32 unused3,
        i32 unused4
    );

    i32 RetireScene(i32 pct, i32 dur, i32 lead, b32 useOverlay);
    i32 BeginSceneFade(i32 intensityPercent, u32 durationMs, u32 leadMs, bool useOverlay);
    i32 AdvanceSceneFade(u32 deltaMs);
    i32 BeginScenePresentation();
    void CancelSceneFade();
    bool IsSceneFading() const { return m_sceneFade.active(); }
    virtual void OnSceneFadeComplete() {}
    virtual i32 RestoreAfterSceneFade() { return InputVirtual(); }

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
        const std::string& name,
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
    i32 m_cursorX;
    i32 m_cursorY;
    i32 m_snapOriginX;
    i32 m_snapOriginY;

    CDDSurface* m_cursorSavedSurfaces[2];

    RECT m_cursorSavedRects[2];
    RECT m_cursorScreenRects[2];

    i32 m_cursorSavedSurfaceValid[2];
    i32 m_cursorBufferIndex;

private:
    FadePlayback m_sceneFade;
    bool m_scenePresentation;
};

inline CState::CState() {
    m_mgr = NULL;
    m_faderMgr = NULL;
    m_scenePresentation = false;
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
    SET_RECT_XY_EXTENTS(m_cursorSavedRects[0], 0, 0x40, 0, 0x40);
    SET_RECT_XY_EXTENTS(m_cursorSavedRects[1], 0, 0x40, 0, 0x40);
    SET_RECT_XY_EXTENTS(m_cursorScreenRects[0], 0, 0, 0, 0);
    SET_RECT_XY_EXTENTS(m_cursorScreenRects[1], 0, 0, 0, 0);
    m_cursorX = 0;
    m_cursorY = 0;
}

#endif
