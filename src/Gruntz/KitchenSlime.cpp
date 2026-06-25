#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CKitchenSlime : CUserLogic) for the leaf-dtor fold
// KitchenSlime.cpp - CKitchenSlime::LoadSprites @0x0b3160 (C:\Proj\Gruntz). The
// kitchen-slime hazard's per-step "advance to the next walkable tile" driver: it
// probes up to four tiles in the slime's current travel direction (m_10->m_124),
// stopping at the first in-bounds, walkable, on-screen tile; rotates the travel
// direction when blocked; then sets the movement vector + direction sprite, the
// per-tile timing (Hazardz/KitchenSlimeTimePerTile butemgr default), and caches
// the new direction sprite's first frame. Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
// CButeMgr g_buteMgr / the CUserLogic base hierarchy come from <Gruntz/UserLogic.h>.

struct CSprite;

// The sub-object embedded in the anim player at +0x1a0 (a CSubMgr-style member);
// Tick advances it once per frame via its 0x55c360 method (one int arg).
struct CSlimeSubMgr {
    void Advance(i32 tick); // CDDrawSubMgr method @0x55c360 (reloc-masked)
};

// The animation player @this+0x38 that holds the current direction sprite at
// +0x194 and the cached first-frame trio at +0x190/+0x194/+0x198.
struct CSlimeAnimPlayer {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame (reloc-masked)

    char m_pad0[0x8];
    i32 m_8; // +0x08  status/flags word (Tick sets bit 0x10000 when stalled)
    char m_padc[0x190 - 0xc];
    i32 m_190;          // +0x190  first frame number
    CSprite* m_194;     // +0x194  the current direction sprite
    i32* m_198;         // +0x198  first frame pointer
    char m_pad19c[4];   // +0x19c
    CSlimeSubMgr m_1a0; // +0x1a0  per-frame sub-mgr (Advance)
};

// The looked-up direction sprite (frame table @+0x14, valid range [m_64..m_68]).
struct CSprite {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame

    char m_pad0[0x14];
    i32** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64
    i32 m_68; // +0x68
};

// The slime's resource/level holder (this->m_10). m_124 = travel direction
// (1..4); m_5c/m_60 = per-step pixel deltas; m_134..m_140 = the on-screen tile
// window; m_12c = a "lock direction" flag; m_7c = the level/timing object whose
// +0xbc overrides the per-tile time.
// The level/timing object at CSlimeLevel+0x7c; its +0xbc overrides the per-tile time.
struct CSlimeTiming {
    char m_pad0[0xbc];
    u32 m_bc;
};

struct CSlimeLevel {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  step dx (pixels)
    i32 m_60; // +0x60  step dy (pixels)
    char m_pad64[0x7c - 0x64];
    CSlimeTiming* m_7c; // +0x7c  level/timing object (m_7c->m_bc time override)
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x12c - 0x128];
    i32 m_12c; // +0x12c lock-direction flag
    char m_pad130[0x134 - 0x130];
    i32 m_134; // +0x134 window min X
    i32 m_138; // +0x138 window min Y
    i32 m_13c; // +0x13c window max X
    i32 m_140; // +0x140 window max Y
    i32 m_144; // +0x144 on-screen rect base (Tick passes &m_144 to the cue gate)
};

// The level tile map reached via g_gameReg->m_70. m_c/m_10 = grid extents,
// m_8 = the row table (row[gy][gx*7] is the tile-flags word).
struct CTileMap {
    char m_pad0[0x8];
    i32** m_8; // +0x08  row table
    i32 m_c;   // +0x0c  grid width
    i32 m_10;  // +0x10  grid height
};
// The on-screen object reached as g_gameReg->m_68 (the visibility/cue gate). Its
// QueryAt resolves the entity under the slime's screen rect and ScrollTo posts a
// scroll; modeled NO-body so both calls reloc-mask.
struct CSlimeCueGate {
    // QueryAt(level->m_5c, level->m_60, &level->m_144, &outA, &outB, 0) -> entity*.
    void* QueryAt(i32 x, i32 y, i32* rect, i32* outA, i32* outB, i32 z); // 0x75c60
    void ScrollTo(i32 a, i32 b, i32 mode, i32 flags);                    // 0x6bcb0
};

struct CGameReg {
    char m_pad0[0x68];
    CSlimeCueGate* m_68; // +0x68  on-screen visibility/cue gate
    char m_pad6c[0x70 - 0x6c];
    CTileMap* m_70; // +0x70
    char m_pad74[0x118 - 0x74];
    i32 m_118; // +0x118 has-window flag
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134 mode discriminator (==1 -> skip the visibility scroll)
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// The entity QueryAt returns; +0x258 is its type/state tag (0x38 == the slime
// itself, so its own footprint is ignored when probing the destination tile).
struct CSlimeEntity {
    char m_pad0[0x258];
    i32 m_258; // +0x258 type tag
};

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator).
DATA(0x001ea3e0)
extern const double g_slimeSpeedNum; // VA 0x5ea3e0

// Per-frame scroll/scale factor (.data int) Tick multiplies into m_58 to get the
// per-frame pixel step.
DATA(0x00245584)
extern i32 g_slimeFrameScale; // VA 0x645584

// 0.0 (the velocity-sign comparand for the movement integrator).
DATA(0x001ea400)
extern const double g_slimeZero; // VA 0x5ea400

// A frame/tick counter (BSS) the anim sub-mgr Advance consumes.
DATA(0x002bf3bc)
extern i32 g_slimeTick; // VA 0x6bf3bc

// CKitchenSlime : CUserLogic - a kitchen-slime hazard game object. The bound
// CGameObject is held at the inherited m_10/m_38 (CUserLogic sets m_10==m_38==
// obj); the slime views it as CSlimeLevel/CSlimeAnimPlayer (a typed reinterpret
// of the same object - the bodies cast m_10/m_38 at each use, codegen-neutral
// since the slime fields overlay the same offsets). The leaf adds the
// movement-integrator state at +0x58. The CUserLogic base gives the +0x18
// destructible link, so ~CKitchenSlime folds the shared teardown (the /GX
// leaf-dtor archetype, see UserLogic.cpp 0x10ab0).
class CKitchenSlime : public CUserLogic {
public:
    void FireActivation(i32 coord);
    i32 Tick();
    i32 LoadSprites();
    ~CKitchenSlime(); // 0x013100 (folds the CUserLogic teardown)

    char m_pad40[0x58 - 0x40];
    double m_58; // +0x58  per-frame speed
    double m_60; // +0x60  accumulated dx (double)
    double m_68; // +0x68  accumulated dy (double)
    double m_70; // +0x70  (cleared)
    double m_78; // +0x78  (cleared)
    i32 m_80;    // +0x80  current tile X
    i32 m_84;    // +0x84  current tile Y
    i32 m_88;    // +0x88  (per-step magnitude / cleared)
    i32 m_8c;    // +0x8c  (cleared)
};

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CKitchenSlime::FireActivation
// (0x0b2940) dispatches through - the SAME archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150, see UserLogic.cpp), but a
// DIFFERENT registry instance (the slime's, at 0x646228 vs the trigger's at
// 0x244688). A coordinate maps to an Entry* either directly (when within the
// fast [g_kslimeLo,g_kslimeHi] range) via g_kslimeBase + (coord-lo)*stride, or
// by a slow Find in the collection (0x16da80, __thiscall ret 8), which on miss
// rebuilds (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret
// 0xc) and yields g_kslimeCur. The entry's first dword is a fn-ptr; a nonzero
// entry's handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (shared with the trigger registry's engine functions). The
// alloc-cache pair (g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) is the
// SAME shared global both registries write.
struct CKSlimeEntry; // an entry: first dword is the registered handler
struct CKSlimeColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CKSlimeColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x00246230)
extern i32 g_kslimeLo;
DATA(0x00246234)
extern i32 g_kslimeHi;
DATA(0x00246238)
extern char* g_kslimeBase;
DATA(0x00246240)
extern i32 g_kslimeStride;
DATA(0x0024623c)
extern CKSlimeEntry* g_kslimeCur;
DATA(0x00246248)
extern i32 g_kslimeScratch;
DATA(0x00246228)
extern CKSlimeColl g_kslimeColl;
DATA(0x0024622c)
extern CKSlimeColl2* g_kslimeColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CKitchenSlime
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CKitchenSlime is defined
// COMPLETE above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CKitchenSlime::*KSlimeHandler)();
struct CKSlimeEntry {
    KSlimeHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CKSlimeEntry* KSlimeLookup(i32 coord) {
    g_kslimeScratch = 0;
    if (coord >= g_kslimeLo && coord <= g_kslimeHi) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    if (g_kslimeColl.Find(coord, 0)) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_kslimeColl2->Insert(&g_kslimeColl, item, 0xc);
    return g_kslimeCur;
}

// The math externs the movement integrator chains (CRT, reloc-masked):
//   double floor(double) = 0x120580, double ceil(double) = 0x120480; the (int)
//   casts lower to __ftol (0x11f570); fabs lowers inline (d9 e1).
extern "C" double floor(double);
extern "C" double ceil(double);
extern "C" double fabs(double);

// CKitchenSlime::~CKitchenSlime @0x013100 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to the established leaf
// dtors (UserLogic.cpp 0x10ab0 / 0x11540); the empty body is enough for cl.
RVA(0x00013100, 0x44)
CKitchenSlime::~CKitchenSlime() {}

// CKitchenSlime::FireActivation @0x0b2940 - look the activation coordinate up in
// the slime's per-coordinate registry; if the entry has a registered handler,
// look it up again and dispatch it __thiscall on this. Same archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150).
RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CKSlimeEntry* e = KSlimeLookup(coord);
    if (e->m_fn != 0) {
        CKSlimeEntry* e2 = KSlimeLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CKitchenSlime::Tick @0x0b2ca0 - the per-frame driver. Advances the anim
// sub-mgr, runs the on-screen visibility/scroll gate (unless the registry is in
// the no-scroll mode), and if the slime has reached its destination tile asks
// LoadSprites for the next leg; otherwise integrates the sub-pixel movement
// vector (m_70/m_78 unit signs * the per-frame step) into m_60/m_68, snapping to
// the target tile on overshoot and writing the new grid position back to m_10.
// The integer scaffolding + visibility/already-arrived blocks are byte-exact.
// @early-stop
// x87 FP movement-integrator wall (docs/patterns/x87-fp-stack-schedule.md): the
// residual is a stack-slot swap (MSVC parks `step` at [esp+8] vs retail's
// [esp+0x10], swapping the per-iter temp) plus the dead x-clamp redundant-jump
// schedule. Logic byte-for-byte correct; ~95%, above the documented 60-75% range.
RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    ((CSlimeAnimPlayer*)m_38)->m_1a0.Advance(g_slimeTick);

    CGameReg* reg = g_gameReg;
    if (reg->m_118 == 0 || reg->m_134 != 1) {
        CSlimeLevel* lvl = (CSlimeLevel*)m_10;
        i32 outX, outY;
        CSlimeEntity* ent =
            (CSlimeEntity*)reg->m_68->QueryAt(lvl->m_5c, lvl->m_60, &lvl->m_144, &outY, &outX, 0);
        if (ent && ent->m_258 != 0x38) {
            g_gameReg->m_68->ScrollTo(outY, outX, 5, -1);
        }
    }

    CSlimeLevel* lvl = (CSlimeLevel*)m_10;
    if (lvl->m_5c == m_80 && lvl->m_60 == m_84 && LoadSprites() == 0) {
        ((CSlimeAnimPlayer*)m_38)->m_8 |= 0x10000;
        return 0;
    }

    double step = (double)(i64)(u64)(u32)g_slimeFrameScale * m_58;
    double* m88d = (double*)&m_88;

    i32 newX;
    if (m_70 > g_slimeZero) {
        double t = (m_60 = m_60 + step);
        newX = (i32)floor(t);
        i32 tx = m_80;
        *m88d = fabs(m_60 - (double)tx);
        // The X axis never clamps (unlike Y), but retail still emits the compare
        // (a min/max fold whose result equals the input); the empty-body test
        // reproduces the cmp + m_80 stack-spill shared with the fabs.
        if (newX > tx) {
            newX = newX;
        }
    } else if (m_70 < g_slimeZero) {
        double t = (m_60 = m_60 - step);
        newX = (i32)ceil(t);
        i32 tx = m_80;
        *m88d = fabs(m_60 - (double)tx);
        if (newX < tx) {
            newX = newX;
        }
    } else {
        newX = (i32)floor(m_60);
    }

    i32 newY;
    if (m_78 > g_slimeZero) {
        double t = (m_68 = m_68 + step);
        newY = (i32)floor(t);
        i32 ty = m_84;
        *m88d = fabs(m_68 - (double)ty);
        if (newY > ty) {
            ((CSlimeLevel*)m_10)->m_5c = newX;
            ((CSlimeLevel*)m_10)->m_60 = ty;
            return 0;
        }
    } else if (m_78 < g_slimeZero) {
        double t = (m_68 = m_68 - step);
        newY = (i32)ceil(t);
        i32 ty = m_84;
        *m88d = fabs(m_68 - (double)ty);
        if (newY < ty) {
            ((CSlimeLevel*)m_10)->m_5c = newX;
            ((CSlimeLevel*)m_10)->m_60 = ty;
            return 0;
        }
    } else {
        newY = (i32)floor(m_68);
    }

    ((CSlimeLevel*)m_10)->m_5c = newX;
    ((CSlimeLevel*)m_10)->m_60 = newY;
    return 0;
}

// @early-stop
// Returns int (1 on success, 0 when no walkable tile was found) - the true
// signature, needed by Tick's `LoadSprites() == 0` test. Residual is the same FP
// /jump-table stack-frame schedule wall it has carried (retail reserves 0x1c vs
// our 0x14 - an extra direction-magnitude stack temp). ~69%, logic exact.
RVA(0x000b3160, 0x339)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = ((CSlimeLevel*)m_10)->m_124;

    i32 tileX, tileY;
    i32 found = 0;
    for (i32 i = 0; i <= 4;) {
        CSlimeLevel* lvl = (CSlimeLevel*)m_10;
        i32 sw = lvl->m_124 - 1;
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

        i32 gx = tileX >> 5;
        i32 gy = tileY >> 5;
        i32 tileFlags;
        CTileMap* map = g_gameReg->m_70;
        if ((u32)gx >= (u32)map->m_c || (u32)gy >= (u32)map->m_10) {
            tileFlags = 1;
        } else {
            tileFlags = ((i32*)map->m_8[gy])[gx * 7];
        }

        if (tileY >= lvl->m_138 && tileX <= lvl->m_13c && tileY <= lvl->m_140 && tileX >= lvl->m_134
            && !(tileFlags & 0x939) && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return 0;
        }

        if (lvl->m_12c == 1) {
            lvl->m_124 = sw;
            if (((CSlimeLevel*)m_10)->m_124 <= 0) {
                ((CSlimeLevel*)m_10)->m_124 = 4;
            }
        } else {
            lvl->m_124++;
            if (((CSlimeLevel*)m_10)->m_124 > 4) {
                ((CSlimeLevel*)m_10)->m_124 = 1;
            }
        }
    }
    if (!found) {
        return 0;
    }

    m_60 = 0;
    m_68 = 0;
    i32 changed = (((CSlimeLevel*)m_10)->m_124 != savedDir);
    switch (((CSlimeLevel*)m_10)->m_124 - 1) {
        case 0: // north
            m_68 = -(double)*(i32*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *(i32*)&m_78 = 0;
            *((i32*)&m_70 + 1) = 0;
            *((i32*)&m_78 + 1) = 0xbff00000;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case 1: // east
            *(i32*)&m_60 = m_88;
            *((i32*)&m_60 + 1) = *((i32*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_70 + 1) = 0x3ff00000;
            *((i32*)&m_78 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case 2: // south
            *(i32*)&m_68 = m_88;
            *((i32*)&m_68 + 1) = *((i32*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_78 + 1) = 0x3ff00000;
            *((i32*)&m_70 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_60 = -(double)*(i32*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_70 + 1) = 0xbff00000;
            *((i32*)&m_78 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_60 = (double)((CSlimeLevel*)m_10)->m_5c + m_60;
    m_68 = (double)((CSlimeLevel*)m_10)->m_60 + m_68;

    u32 time;
    if (((CSlimeLevel*)m_10)->m_7c->m_bc != 0) {
        time = ((CSlimeLevel*)m_10)->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_58 = g_slimeSpeedNum / (double)(i64)(u64)time;
    m_80 = tileX;
    m_84 = tileY;

    CSlimeAnimPlayer* player = (CSlimeAnimPlayer*)m_38;
    CSprite* spr = player->m_194;
    if (changed != 0 && spr != 0) {
        if (spr->m_64 <= 1 && spr->m_68 >= 1) {
            player->m_190 = 1;
            player->m_198 = spr->m_14[1];
            m_88 = 0;
            m_8c = 0;
            return 1;
        }
        player->m_190 = 1;
        player->m_198 = 0;
        m_88 = 0;
        m_8c = 0;
        return 1;
    }
    m_88 = 0;
    m_8c = 0;
    return 1;
}
// size 0x90 from operator-new vtable attribution (gruntz.analysis.news)
SIZE(CKitchenSlime, 0x90);
