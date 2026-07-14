// BridgeMoveSprites.cpp - CPlayLevelLoad::LoadBridgeMove (RVA 0x110860), the
// frameless sibling of CPlayLevelLoad::LoadPyramidBridge (0x110c10). Given a tile-
// action descriptor (`this`, +0x8 = tile x, +0xc = tile y) and a sprite-type id,
// it dispatches a 0x66-case jump table over (type - 0xf): each in-bounds case
// plays the matching bridge-transition sound cue. Two cue shapes:
//   * FindEntry + rate-limited Play(g_sndCueTag) (GAME_PYRAMIDMOVE /
//     LEVEL_WATERBRIDGEMOVE), gated on the sound-set guard (set->m_30 == 0);
//   * a bare PlaySimple(name) (LEVEL_WATERBRIDGEMOVE / LEVEL_DEATHBRIDGEMOVE /
//     LEVEL_CRUMBLE), no guard.
// The bounds gate clamps the descriptor's pixel origin (tile<<5 + 0x10) into the
// game-reg map rect (m_13c..m_144 / m_140..m_148). Only offsets / code bytes are
// load-bearing; the sound-chain helpers + the GAME_*/LEVEL_* strings are reloc-
// masked externals/$SG literals on the same singleton (*0x64556c) as its siblings.

#include <rva.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/GameRegistry.h> // g_gameReg canonical view (0x24556c)

// The booty/bridge sound chain on the *0x64556c game registry (the same shape the
// BootyState/CHelpBookSprite cue idioms use). The former BmGameReg/BmSndMgr local
// views are dissolved onto the canonical CGameRegistry: the rect is m_viewOrigin*
// (+0x13c..+0x148) and the sound chain is m_world->m_28 (CSndHost == CDDrawSubMgrLeafScan).
extern "C" CGameRegistry* g_gameReg; // 0x64556c
extern i32 g_sndCueTag;              // ?g_sndCueTag@@3HA @0x61ab24

class CPlayLevelLoad {
public:
    void LoadBridgeMove(i32 type);
    char m_pad00[0x8];
    i32 m_8; // +0x08  tile x
    i32 m_c; // +0x0c  tile y
};

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
void CPlayLevelLoad::LoadBridgeMove(i32 type) {
    i32 px, py;
    CGameRegistry* r;
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
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                set = r->m_world->m_28;
                if (set->m_30 == 0) {
                    LeafCue* e = (LeafCue*)set->Lookup_05b7e0("GAME_PYRAMIDMOVE");
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 107:
        case 108:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                set = r->m_world->m_28;
                if (set->m_30 == 0) {
                    LeafCue* e = (LeafCue*)set->Lookup_05b7e0("LEVEL_WATERBRIDGEMOVE");
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 113:
        case 114:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_28->RefreshAsset_114120("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case 109:
        case 110:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_28->RefreshAsset_114120("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 115:
        case 116:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_28->RefreshAsset_114120("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 111:
        case 112:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_gameReg;
            if (px < r->m_viewOriginR && px >= r->m_viewOriginL && py < r->m_viewOriginB
                && py >= r->m_viewOriginT) {
                r->m_world->m_28->RefreshAsset_114120("LEVEL_CRUMBLE");
            }
            return;
    }
}

SIZE_UNKNOWN(CPlayLevelLoad);
