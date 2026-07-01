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

// The booty/bridge sound chain on the *0x64556c game registry (the same shape the
// BootyState/CHelpBookSprite cue idioms use).
struct BmSndEntry {
    void Play(i32 token, i32 a, i32 b, i32 c); // 0x25fe __thiscall
};
struct BmSndSet {
    BmSndEntry* FindEntry(char* name); // 0x2cca __thiscall -> entry (0 if absent)
    void PlaySimple(char* name);       // 0x226b __thiscall, void
    char m_pad00[0x30];
    i32 m_30; // +0x30  active guard
};
struct BmSndMgr {
    char m_pad00[0x28];
    BmSndSet* m_28; // +0x28
};
struct BmGameReg {
    char m_pad00[0x30];
    BmSndMgr* m_30; // +0x30  sound mgr
    char m_pad34[0x13c - 0x34];
    i32 m_13c; // +0x13c  rect x-lo
    i32 m_140; // +0x140  rect y-lo
    i32 m_144; // +0x144  rect x-hi
    i32 m_148; // +0x148  rect y-hi
};
extern "C" BmGameReg* g_mgrSettings; // _g_mgrSettings @0x64556c
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
    BmGameReg* r;
    BmSndSet* set;
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
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                set = r->m_30->m_28;
                if (set->m_30 == 0) {
                    BmSndEntry* e = set->FindEntry("GAME_PYRAMIDMOVE");
                    if (e) {
                        e->Play(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 107:
        case 108:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                set = r->m_30->m_28;
                if (set->m_30 == 0) {
                    BmSndEntry* e = set->FindEntry("LEVEL_WATERBRIDGEMOVE");
                    if (e) {
                        e->Play(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case 113:
        case 114:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                r->m_30->m_28->PlaySimple("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case 109:
        case 110:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                r->m_30->m_28->PlaySimple("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 115:
        case 116:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                r->m_30->m_28->PlaySimple("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case 111:
        case 112:
            py = (m_c << 5) + 0x10;
            px = (m_8 << 5) + 0x10;
            r = g_mgrSettings;
            if (px < r->m_144 && px >= r->m_13c && py < r->m_148 && py >= r->m_140) {
                r->m_30->m_28->PlaySimple("LEVEL_CRUMBLE");
            }
            return;
    }
}

SIZE_UNKNOWN(BmGameReg);
SIZE_UNKNOWN(BmSndEntry);
SIZE_UNKNOWN(BmSndMgr);
SIZE_UNKNOWN(BmSndSet);
SIZE_UNKNOWN(CPlayLevelLoad);
