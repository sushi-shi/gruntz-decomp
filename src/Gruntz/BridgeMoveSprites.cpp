#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/GameRegistry.h>     // g_gameReg canonical view (0x24556c)
#include <Gruntz/TileTriggerLogic.h> // this IS CTileTriggerLogic (m_08/m_0c coord x/y)

// @early-stop
// switch range-header + inline-jump-table wall (~77%): all six case bodies are
// byte-exact vs retail (bounds gate, the FindEntry+Play(g_sndCueTag) cue arms and
// the bare PlaySimple arms; reg assignment matched by computing py(m_c) before
// px(m_8) so m_c lands in eax). Residual: (1) retail's switch normalizes over
// [15,116] (`add -0xf; cmp 0x65`) while MSVC tightens mine to [93,116]
// (`add -0x5d; cmp 0x17`) - the original had explicit empty low cases (15..18 ->
// the slot-0 default, 19..92 the slot-7 gap-default in the index table) but any
// `case 15..18: return;` group makes MSVC emit a real block instead of tail-
// merging to default, misaligning every body (drops to ~27%); (2) the .rdata
// jump+index tables are emitted inline in the base .text but delinked as separate
// symbols. Both documented: docs/patterns/switch-jumptable-separate-comdat.md +
// jumptable-data-overlap.md. Logic complete; not source-steerable.
RVA(0x00110860, 0x25f)
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
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                set = r->m_world->m_soundRegistry;
                if (set->m_30 == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup_05b7e0("GAME_PYRAMIDMOVE"));
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
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                set = r->m_world->m_soundRegistry;
                if (set->m_30 == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup_05b7e0("LEVEL_WATERBRIDGEMOVE"));
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
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_soundRegistry->RefreshAsset_114120("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case 109:
        case 110:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_soundRegistry->RefreshAsset_114120("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 115:
        case 116:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_soundRegistry->RefreshAsset_114120("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 111:
        case 112:
            py = (m_tileY << 5) + 0x10;
            px = (m_tileX << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_soundRegistry->RefreshAsset_114120("LEVEL_CRUMBLE");
            }
            return;
    }
}
