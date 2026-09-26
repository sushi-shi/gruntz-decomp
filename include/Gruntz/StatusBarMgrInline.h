#ifndef GRUNTZ_GRUNTZ_STATUSBARMGRINLINE_H
#define GRUNTZ_GRUNTZ_STATUSBARMGRINLINE_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzWnd.h>
#include <Gruntz/Play.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StatusBarMgr.h>

static __inline void HiCueFind() {
    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
    if (registry->m_silentMode == false) {
        CObject* obj = registry->Lookup("GAME_TABHIGHLIGHT1");
        if (obj) {
            static_cast<SoundCue*>(obj)->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
        }
    }
}

static __inline void HiCueLookup() {
    g_gameReg->m_world->m_soundRegistry->PlayCue("GAME_TABHIGHLIGHT1");
}

static __inline void HiCueTimed() {
    PlayRegistryCueIfElapsed(g_gameReg->m_world->m_soundRegistry, "GAME_TABHIGHLIGHT1");
}

static __inline void PlayTabCue(CStatusBarMgr* statusBar, StatusBarTab tab, const char* cueKey) {
    if (statusBar->m_activeTab == tab && statusBar->m_position != STATUSBAR_HIDDEN) {
        PlayRegistryCueIfElapsed(g_gameReg->m_world->m_soundRegistry, cueKey);
    }
}

static __inline void HiPost(i32 cmdId) {
    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, cmdId, 0);
}

inline b32 CStatusBarMgr::ActivateReadySlot(i32 slot) {
    if (!(static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
        return false;
    }
    HiCueTimed();
    m_activeSlot = slot;
    m_slots[slot].m_value = 1;
    if (m_slotNotify[slot]) {
        m_slotNotify[slot]->Notify(1);
    }
    return true;
}

#endif // GRUNTZ_GRUNTZ_STATUSBARMGRINLINE_H
