#include <rva.h>
#include <Bute/ButeMgr.h>
// KitchenSlime.cpp - CKitchenSlime::LoadSprites @0x0b3160 (C:\Proj\Gruntz). The
// kitchen-slime hazard's per-step "advance to the next walkable tile" driver: it
// probes up to four tiles in the slime's current travel direction (m_10->m_124),
// stopping at the first in-bounds, walkable, on-screen tile; rotates the travel
// direction when blocked; then sets the movement vector + direction sprite, the
// per-tile timing (Hazardz/KitchenSlimeTimePerTile butemgr default), and caches
// the new direction sprite's first frame. Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.

extern CButeMgr g_buteMgr;

struct CSprite;

// The animation player @this+0x38 that holds the current direction sprite at
// +0x194 and the cached first-frame trio at +0x190/+0x194/+0x198.
struct CSlimeAnimPlayer {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame (reloc-masked)

    char m_pad0[0x190];
    int m_190;      // +0x190  first frame number
    CSprite* m_194; // +0x194  the current direction sprite
    int* m_198;     // +0x198  first frame pointer
};

// The looked-up direction sprite (frame table @+0x14, valid range [m_64..m_68]).
struct CSprite {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame

    char m_pad0[0x14];
    int** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    int m_64; // +0x64
    int m_68; // +0x68
};

// The slime's resource/level holder (this->m_10). m_124 = travel direction
// (1..4); m_5c/m_60 = per-step pixel deltas; m_134..m_140 = the on-screen tile
// window; m_12c = a "lock direction" flag; m_7c = the level/timing object whose
// +0xbc overrides the per-tile time.
// The level/timing object at CSlimeLevel+0x7c; its +0xbc overrides the per-tile time.
struct CSlimeTiming {
    char m_pad0[0xbc];
    unsigned int m_bc;
};

struct CSlimeLevel {
    char m_pad0[0x5c];
    int m_5c; // +0x5c  step dx (pixels)
    int m_60; // +0x60  step dy (pixels)
    char m_pad64[0x7c - 0x64];
    CSlimeTiming* m_7c; // +0x7c  level/timing object (m_7c->m_bc time override)
    char m_pad80[0x124 - 0x80];
    int m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x12c - 0x128];
    int m_12c; // +0x12c lock-direction flag
    char m_pad130[0x134 - 0x130];
    int m_134; // +0x134 window min X
    int m_138; // +0x138 window min Y
    int m_13c; // +0x13c window max X
    int m_140; // +0x140 window max Y
};

// The level tile map reached via g_gameReg->m_70. m_c/m_10 = grid extents,
// m_8 = the row table (row[gy][gx*7] is the tile-flags word).
struct CTileMap {
    char m_pad0[0x8];
    int** m_8; // +0x08  row table
    int m_c;   // +0x0c  grid width
    int m_10;  // +0x10  grid height
};
struct CGameReg {
    char m_pad0[0x70];
    CTileMap* m_70; // +0x70
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator).
DATA(0x001ea3e0)
extern const double g_slimeSpeedNum; // VA 0x5ea3e0

class CKitchenSlime {
public:
    void LoadSprites();

    char m_pad0[0x10];
    CSlimeLevel* m_10; // +0x10
    char m_pad14[0x38 - 0x14];
    CSlimeAnimPlayer* m_38; // +0x38
    char m_pad3c[0x58 - 0x3c];
    double m_58; // +0x58  per-frame speed
    double m_60; // +0x60  accumulated dx (double)
    double m_68; // +0x68  accumulated dy (double)
    double m_70; // +0x70  (cleared)
    double m_78; // +0x78  (cleared)
    int m_80;    // +0x80  current tile X
    int m_84;    // +0x84  current tile Y
    int m_88;    // +0x88  (per-step magnitude / cleared)
    int m_8c;    // +0x8c  (cleared)
};

RVA(0x000b3160, 0x339)
void CKitchenSlime::LoadSprites() {
    int savedDir = m_10->m_124;

    int tileX, tileY;
    int found = 0;
    for (int i = 0; i <= 4;) {
        CSlimeLevel* lvl = m_10;
        int sw = lvl->m_124 - 1;
        switch (sw) {
            case 0:
                tileX = m_80;
                tileY = m_84 - 0x20;
                break; // north
            case 1:
                tileX = m_80 + 0x20;
                tileY = m_84;
                break; // east
            case 2:
                tileX = m_80;
                tileY = m_84 + 0x20;
                break; // south
            case 3:
                tileX = m_80 - 0x20;
                tileY = m_84;
                break; // west
        }

        int gx = tileX >> 5;
        int gy = tileY >> 5;
        int tileFlags;
        CTileMap* map = g_gameReg->m_70;
        if ((unsigned)gx >= (unsigned)map->m_c || (unsigned)gy >= (unsigned)map->m_10) {
            tileFlags = 1;
        } else {
            tileFlags = ((int*)map->m_8[gy])[gx * 7];
        }

        if (tileY >= lvl->m_138 && tileX <= lvl->m_13c && tileY <= lvl->m_140 && tileX >= lvl->m_134
            && !(tileFlags & 0x939) && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return;
        }

        if (lvl->m_12c == 1) {
            lvl->m_124 = sw;
            if (m_10->m_124 <= 0) {
                m_10->m_124 = 4;
            }
        } else {
            lvl->m_124++;
            if (m_10->m_124 > 4) {
                m_10->m_124 = 1;
            }
        }
    }
    if (!found) {
        return;
    }

    m_60 = 0;
    m_68 = 0;
    int changed = (m_10->m_124 != savedDir);
    switch (m_10->m_124 - 1) {
        case 0: // north
            m_68 = -(double)*(int*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *(int*)&m_78 = 0;
            *((int*)&m_70 + 1) = 0;
            *((int*)&m_78 + 1) = 0xbff00000;
            if (changed) {
                m_38->CacheFirstFrame("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case 1: // east
            *(int*)&m_60 = m_88;
            *((int*)&m_60 + 1) = *((int*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((int*)&m_70 + 1) = 0x3ff00000;
            *((int*)&m_78 + 1) = 0;
            if (changed) {
                m_38->CacheFirstFrame("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case 2: // south
            *(int*)&m_68 = m_88;
            *((int*)&m_68 + 1) = *((int*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((int*)&m_78 + 1) = 0x3ff00000;
            *((int*)&m_70 + 1) = 0;
            if (changed) {
                m_38->CacheFirstFrame("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_60 = -(double)*(int*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *((int*)&m_70 + 1) = 0xbff00000;
            *((int*)&m_78 + 1) = 0;
            if (changed) {
                m_38->CacheFirstFrame("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_60 = (double)m_10->m_5c + m_60;
    m_68 = (double)m_10->m_60 + m_68;

    unsigned int time;
    if (m_10->m_7c->m_bc != 0) {
        time = m_10->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_58 = g_slimeSpeedNum / (double)(__int64)(unsigned __int64)time;
    m_80 = tileX;
    m_84 = tileY;

    if (changed == 0) {
        m_88 = 0;
        m_8c = 0;
        return;
    }

    CSlimeAnimPlayer* player = m_38;
    CSprite* spr = player->m_194;
    if (!spr) {
        m_88 = 0;
        m_8c = 0;
        return;
    }
    if (spr->m_64 > 1 || spr->m_68 < 1) {
        player->m_190 = 1;
        player->m_198 = 0;
    } else {
        player->m_190 = 1;
        player->m_198 = spr->m_14[1];
    }
    m_88 = 0;
    m_8c = 0;
}
// size 0x90 from operator-new vtable attribution (gruntz.analysis.news)
SIZE(CKitchenSlime, 0x90);
