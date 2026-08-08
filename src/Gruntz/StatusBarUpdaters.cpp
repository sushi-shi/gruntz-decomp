#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>

// @early-stop
RVA(0x00110570, 0xfb)
i32 CTileTriggerSwitchLogic::SwitchDown() {
    CGruntzMgr* reg = g_gameReg;
    CDDrawWorkerHost* g = reg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_tileY] + m_tileX] + 1;
    CDDrawWorkerHost* g2 = reg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_tileY] + m_tileX] = v;
    reg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, v);

    i32 px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (CGameLevel::PointInBounds(&g_gameReg->m_viewBounds, px, py)) {
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
    CGruntzMgr* reg = g_gameReg;
    CDDrawWorkerHost* g = reg->m_world->m_level->m_mainPlane;
    i32 v = g->m_tileGrid[g->m_colOffsets[m_tileY] + m_tileX] - 1;
    CDDrawWorkerHost* g2 = reg->m_world->m_level->m_mainPlane;
    g2->m_tileGrid[g2->m_colOffsets[m_tileY] + m_tileX] = v;
    reg->m_tileGrid->ComputeCellFlags(m_tileX, m_tileY, v);

    i32 px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (CGameLevel::PointInBounds(&g_gameReg->m_viewBounds, px, py)) {
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
