#include <rva.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/GameRegistry.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/StatusBarUpdatersViews.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/TileTriggerSwitchLogic.h>

// @early-stop
RVA(0x00110570, 0xfb)
i32 CTileTriggerSwitchLogic::SwitchDown() {
    CDDrawWorkerHost* g = g_gameReg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_tileY] + m_tileX] + 1;
    CDDrawWorkerHost* g2 = g_gameReg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_tileY] + m_tileX] = v;
    g_gameReg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, v);

    i32 px = (m_tileX << 5) + 0x10;
    i32 py = (m_tileY << 5) + 0x10;
    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
        && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top) {
        CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_cues.Lookup("GAME_SWITCHDOWN", spr_ob);
            LeafCue* spr = static_cast<LeafCue*>(spr_ob);
            if (spr) {
                if (g_sndEnabled != 0
                    && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                    spr->m_lastPlayTime = g_killCueClock;
                    spr->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }
    m_linkGate = 1;
    return 1;
}

// @early-stop
RVA(0x001106b0, 0xf4)
i32 CTileTriggerSwitchLogic::SwitchUp() {
    CDDrawWorkerHost* g = g_gameReg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_tileY] + m_tileX] - 1;
    CDDrawWorkerHost* g2 = g_gameReg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_tileY] + m_tileX] = v;
    g_gameReg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, v);

    i32 px = (m_tileX << 5) + 0x10;
    i32 py = (m_tileY << 5) + 0x10;
    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
        && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top) {
        CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            void* spr_ob = 0;
            h->m_cues.Lookup("GAME_SWITCHUP", spr_ob);
            LeafCue* spr = static_cast<LeafCue*>(spr_ob);
            if (spr) {
                if (g_sndEnabled != 0
                    && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                    spr->m_lastPlayTime = g_killCueClock;
                    spr->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }
    m_linkGate = 0;
    return 1;
}
