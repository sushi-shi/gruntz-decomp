#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TileTriggerLogic.h>

// @early-stop
RVA(0x00110860, 0x2a0)
void CTileTriggerLogic::LoadBridgeMove(i32 type) {
    i32 px, py;
    CGruntzMgr* r;
    CDDrawSubMgrLeafScan* set;
    switch (type) {
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                set = r->m_world->m_soundRegistry;
                if (set->m_emitGate == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup("GAME_PYRAMIDMOVE"));
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 107:
        case 108:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                set = r->m_world->m_soundRegistry;
                if (set->m_emitGate == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup("LEVEL_WATERBRIDGEMOVE"));
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 113:
        case 114:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case 109:
        case 110:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 115:
        case 116:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 111:
        case 112:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_CRUMBLE");
            }
            return;
    }
}
