#include <rva.h>
#include <Gruntz/CString.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory + CSpriteIconNode shape
#include <Bute/ButeMgr.h>
// IconLoaders.cpp - the in-game-icon / powerup / explosion / camera / booty-perfect
// sprite loaders (C:\Proj\Gruntz). Each builds a named sprite-set key, asks the
// global HUD sprite factory (g_gameReg->m_factoryHolder->m_factory->CreateSprite) for the sprite,
// then caches/forwards something off it through the shared sprite-resource leaves
// (CGruntSprite::CacheFirstFrame, CGruntAnimPlayer::ApplyLookupGeometry - both in
// the spriteresource unit, reloc-masked). Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.

// ---------------------------------------------------------------------------
// Shared engine objects, modeled minimally (mirroring SpriteLoaders.cpp's idiom).
// ---------------------------------------------------------------------------
// CSpriteFactory + CSpriteIconNode + the CIconSprite forward-decl now live in the
// shared <Gruntz/SpriteFactory.h> (included above); CIconSprite is defined below.

// Two icon-class vtable-slot fns the toybox de-dup test compares an existing
// icon's CIconSprite::m_initVtbl->Init against (the in-game-icon / in-game-text classes).
// Declared extern so each `cmp esi, OFFSET fn` immediate carries a DIR32 reloc
// that pairs with retail's reloc at 0x40288d / 0x402bad (names are reloc-masked).
extern "C" void IconClassInitA(); // 0x40288d
extern "C" void IconClassInitB(); // 0x402bad

struct CSpriteFactoryHolder {
    char m_pad0[0x8];
    CSpriteFactory* m_factory; // +0x08
};

// The game-manager singleton (g_gameReg, *0x64556c). Different loaders read a
// handful of its slots (viewport +0x8c/+0x90, a level-tracker +0x2c, the
// visible-bounds mode gate +0x134) plus the factory holder +0x30.
struct CResourceTracker {
    char m_pad0[0x1c];
    i32 m_levelNumber; // +0x1c  (Level number source)
};
struct CGameReg {
    void Report(i32 code, i32 sub); // 0x40346d __thiscall (icon-overflow report)

    char m_pad0[0x2c];
    CResourceTracker* m_resourceTracker;   // +0x2c
    CSpriteFactoryHolder* m_factoryHolder; // +0x30
    char m_pad34[0x8c - 0x34];
    i32 m_viewportX; // +0x8c  viewport X
    i32 m_viewportY; // +0x90  viewport Y
    char m_pad94[0x134 - 0x94];
    i32 m_visibleBoundsMode; // +0x134 visible-bounds mode gate (==1)
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// The attribute manager (butemgr unit), reached as the g_buteMgr singleton.
extern CButeMgr g_buteMgr;

// CRT (reloc-masked).
extern "C" i32 rand(void);

// The created sprite. CacheFirstFrame caches its first valid frame; the icon
// loader pushes a pile of configuration ints into the +0x114..+0x130 block. The
// per-sprite animation player @+0x38 carries ApplyLookupGeometry.
// The CIconSprite "init interface" vtable embedded at CIconSprite+0x7c; slot +0x10 is the
// init function the camera loader runs after CreateSprite.
struct CSpriteVtbl {
    void* m_slot0[4];           // +0x00  slots 0..3
    void (*Init)(CIconSprite*); // +0x10  the init "virtual" (takes the sprite)
};

struct CIconSprite {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame @0x150540
    i32 ApplyLookupGeometry(const char* name, i32 applyDefault); // CGruntAnimPlayer @0x1505b0

    char m_pad0[0x40]; // +0x00
    i32 m_flags;       // +0x40  flag bits
    char m_pad44[0x5c - 0x44];
    i32 m_subTileX;             // +0x5c  sub-tile X
    i32 m_subTileY;             // +0x60  sub-tile Y
    char m_pad64[0x7c - 0x64];  // +0x64
    CSpriteVtbl* m_initVtbl;    // +0x7c  init-interface vtable (slot +0x10 = Init)
    char m_pad80[0x114 - 0x80]; // +0x80
    i32 m_114;                  // +0x114
    i32 m_118;                  // +0x118
    i32 m_11c;                  // +0x11c
    i32 m_120;                  // +0x120
    i32 m_124;                  // +0x124
    i32 m_128;                  // +0x128
    i32 m_12c;                  // +0x12c
    i32 m_130;                  // +0x130
};

// the matched leaves live in the spriteresource unit; declared here so the
// `call rel32` reloc-masks (the mangled name + arg shape are load-bearing).
// CGruntSprite::CacheFirstFrame == CIconSprite::CacheFirstFrame here.
// (mangled ?CacheFirstFrame@CGruntSprite@@QAEXPBD@Z / etc. fall out of the names.)

// The format wrapper (NAFXCW CString::Format, called cdecl-style with the dst
// CString by address; reloc-masked). Declared as the member so the call shape
// (push args; push &cstr; call Format) matches.
//   name.Format(fmt, ...) is used directly via CString.

// The free EngineLabelBacklog holder (matches the stub's mangled namespace).
class EngineLabelBacklog {
public:
    i32 LoadPowerupIconSprites(i32 type, i32 geoB, i32 geoA, i32 m130, i32 warpIdx, i32 m120);
    i32 LoadExplosionSprites(i32 geoB, i32 geoA, i32 variant, i32 dummy);
    i32 LoadCameraSprite();
    i32 LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5);
    i32 BuildBootyPerfectAnimation();

    char m_pad00[0x22c];                   // +0x000
    CSpriteFactoryHolder* m_factoryHolder; // +0x22c  sprite-factory holder
    char m_pad230[0x23c - 0x230];          // +0x230
    CIconSprite* m_cameraSprite;           // +0x23c  cached camera sprite
    char m_pad240[0x2f8 - 0x240];          // +0x240
    CIconSprite* m_bootyPerfectSprite;     // +0x2f8  booty-perfect anim sprite
};

// ===========================================================================
// BuildBootyPerfectAnimation @0x01c070
// ===========================================================================
//
// Builds the "BOOTY_PERFECT" celebration animation: CreateSprite a SimpleAnimation
// at a fixed screen position, cache its first frame, then resolve its cycle
// geometry. __thiscall (this @ esi). Stores the sprite at this+0x2f8.

RVA(0x0001c070, 0x59)
i32 EngineLabelBacklog::BuildBootyPerfectAnimation() {
    CIconSprite* spr = g_gameReg->m_factoryHolder->m_factory
                           ->CreateSprite(0, (i32)0xffffff7e, 0xf0, 0x64, "SimpleAnimation", 3);
    m_bootyPerfectSprite = spr;
    if (!spr) {
        return 0;
    }
    spr->CacheFirstFrame("BOOTY_PERFECT");
    m_bootyPerfectSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

// ===========================================================================
// LoadCameraSprite @0x078960
// ===========================================================================
//
// Lazily creates the "DoNothing" camera sprite once (gated on this+0x23c being
// empty). It positions the sprite from the viewport (g_gameReg->m_viewportX/m_viewportY) offset
// by a per-tile bias selected from the current map's first travel count, runs the
// sprite's init virtual (vtbl slot +0x10), then caches its first frame.
// __thiscall (this @ esi). Returns 1 on (re)creation, 0 if already present.

RVA(0x00078960, 0x9b)
i32 EngineLabelBacklog::LoadCameraSprite() {
    if (m_cameraSprite != 0) {
        return 0;
    }

    i32 vx = g_gameReg->m_viewportX;
    i32 vy = g_gameReg->m_viewportY;
    i32 count = *(*(i32**)((char*)g_gameReg->m_resourceTracker + 0x2dc));

    i32 ax, cx;
    if (count == 0) {
        ax = vx - 0xc8;
        cx = vy - 0x28;
    } else if (count > 0 && count <= 2) {
        ax = vx - 0x28;
        cx = vy - 0x28;
    }

    CSpriteFactory* fac = m_factoryHolder->m_factory;
    CIconSprite* spr = fac->CreateSprite(0, ax, cx, 0xf4240, "DoNothing", 1);
    m_cameraSprite = spr;
    spr->m_initVtbl->Init(spr);
    m_cameraSprite->CacheFirstFrame("GAME_CAMERASPRITE");
    return 1;
}

// ===========================================================================
// LoadToyBoxIcon @0x07a3f0
// ===========================================================================
//
// Lazily creates the "GAME_TOYBOX" in-game icon at tile (x>>5, y>>5): first walks
// the factory's live-icon list (m_factoryHolder->m_factory->m_liveIcons) and bails (return 0) if an
// existing icon of one of the two icon classes already sits on that tile;
// otherwise CreateSprite("InGameIcon"), cache its frame, stamp the four config
// slots and the +0x40 visible bit.  __thiscall, ret 0x14 (5 args).
//
// Byte-identical (99.94%): the only residual is the reloc-typing artifact on the
// two `cmp esi, OFFSET fn` de-dup comparisons (base names IconClassInitA/B vs
// retail thunk_FUN_004bf150 / CSingleAnimation; code bytes match).

RVA(0x0007a3f0, 0xd7)
i32 EngineLabelBacklog::LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5) {
    CSpriteFactory* fac = m_factoryHolder->m_factory;
    i32 tx = x >> 5;
    i32 ty = y >> 5;

    CSpriteIconNode* node = fac->m_liveIcons;
    while (node != 0) {
        CSpriteIconNode* cur = node;
        node = node->next;
        CIconSprite* obj = cur->m_sprite;
        void* init = (void*)obj->m_initVtbl->Init;
        if (init == (void*)&IconClassInitA || init == (void*)&IconClassInitB) {
            i32 ox = obj->m_subTileX >> 5;
            i32 oy = obj->m_subTileY >> 5;
            if (tx == ox && ty == oy) {
                return 0;
            }
        }
    }

    CIconSprite* spr = fac->CreateSprite(0, x, y, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        g_gameReg->Report(0x8009, 0x402);
        return 0;
    }
    spr->CacheFirstFrame("GAME_TOYBOX");
    spr->m_118 = a4;
    spr->m_130 = a5;
    spr->m_114 = a3;
    spr->m_flags |= 1;
    return 1;
}

// ===========================================================================
// LoadExplosionSprites @0x07b330
// ===========================================================================
//
// Creates a random "GAME_EXPLOSION%d" explosion sprite via the "Explosion" class
// template, resolves its geometry, and records the chosen variant index at
// this+0x124 with the loaded flag at this+0x114. __thiscall ret 0x10 (4 args).

RVA(0x0007b330, 0xc6)
i32 EngineLabelBacklog::LoadExplosionSprites(i32 geoB, i32 geoA, i32 variant, i32 dummy) {
    CSpriteFactory* fac = m_factoryHolder->m_factory;
    CIconSprite* spr = fac->CreateSprite(0, geoB, geoA, 0, "Explosion", 0x40003);
    if (spr) {
        i32 v = variant;
        if (v == 0) {
            v = (rand(), 1);
        }
        CString key;
        key.Format("GAME_EXPLOSION%d", v);
        spr->ApplyLookupGeometry(key, 0);
        spr->m_124 = v;
        spr->m_114 = 1;
    }
    return spr != 0;
}

// ===========================================================================
// LoadPowerupIconSprites @0x07c620
// ===========================================================================
//
// The big in-game-icon loader: a switch on the powerup/toy/tool type id selects
// the sprite-set key, then the common path CreateSprite("InGameIcon", ...) +
// CacheFirstFrame + zeroes the +0x114-block. Two special cases: WARPSTONE (a
// per-level key resolved through g_buteMgr) and the closing TIMEBOMB type that
// runs its own CreateSprite("TimeBomb", ...) + a CoveredTimeBombTime default.
// __thiscall-free ret 0x18 (6 args). Returns 1 on success.

RVA(0x0007c620, 0x3c5)
i32 EngineLabelBacklog::LoadPowerupIconSprites(
    i32 type,
    i32 geoB,
    i32 geoA,
    i32 m130,
    i32 warpIdx,
    i32 m120
) {
    if (type == 0) {
        return 0;
    }

    CString name;
    switch (type) {
        case 1:
            name = "GAME_INGAMEICONZ_TOOLZ_BOMBZ";
            break;
        case 2:
            name = "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ";
            break;
        case 3:
            name = "GAME_INGAMEICONZ_TOOLZ_BRICKZ";
            break;
        case 4:
            name = "GAME_INGAMEICONZ_TOOLZ_CLUBZ";
            break;
        case 5:
            name = "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ";
            break;
        case 6:
            name = "GAME_INGAMEICONZ_TOOLZ_GLOVEZ";
            break;
        case 7:
            name = "GAME_INGAMEICONZ_TOOLZ_GOOBERZ";
            break;
        case 8:
            name = "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ";
            break;
        case 9:
            name = "GAME_INGAMEICONZ_TOOLZ_GUNHATZ";
            break;
        case 10:
            name = "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ";
            break;
        case 11:
            name = "GAME_INGAMEICONZ_TOOLZ_ROCKZ";
            break;
        case 12:
            name = "GAME_INGAMEICONZ_TOOLZ_SHIELDZ";
            break;
        case 13:
            name = "GAME_INGAMEICONZ_TOOLZ_SHOVELZ";
            break;
        case 14:
            name = "GAME_INGAMEICONZ_TOOLZ_SPRINGZ";
            break;
        case 15:
            name = "GAME_INGAMEICONZ_TOOLZ_SPYZ";
            break;
        case 16:
            name = "GAME_INGAMEICONZ_TOOLZ_SWORDZ";
            break;
        case 17:
            name = "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ";
            break;
        case 18:
            name = "GAME_INGAMEICONZ_TOOLZ_TOOBZ";
            break;
        case 19:
            name = "GAME_INGAMEICONZ_TOOLZ_WANDZ";
            break;
        case 20:
            if (g_gameReg->m_visibleBoundsMode == 1) {
                CResourceTracker* rt = g_gameReg->m_resourceTracker;
                CString lvl;
                lvl.Format("Level%i", rt->m_levelNumber);
                name.Format(
                    "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i",
                    g_buteMgr.GetInt("WarpStone", (char*)(const char*)lvl)
                );
            } else {
                name.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", warpIdx);
            }
            break;
        case 21:
            name = "GAME_INGAMEICONZ_TOOLZ_WELDERZ";
            break;
        case 22:
            name = "GAME_INGAMEICONZ_TOOLZ_WINGZ";
            break;
        case 23:
            name = "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ";
            break;
        case 24:
            name = "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ";
            break;
        case 25:
            name = "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ";
            break;
        case 26:
            name = "GAME_INGAMEICONZ_TOYZ_GOKARTZ";
            break;
        case 27:
            name = "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ";
            break;
        case 28:
            name = "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ";
            break;
        case 29:
            name = "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ";
            break;
        case 30:
            name = "GAME_INGAMEICONZ_TOYZ_SCROLLZ";
            break;
        case 31:
            name = "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ";
            break;
        case 32:
            name = "GAME_INGAMEICONZ_TOYZ_YOYOZ";
            break;
        case 50:
            name = "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ";
            break;
        case 51:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH3";
            break;
        case 52:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH2";
            break;
        case 53:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH1";
            break;
        case 57:
            name = "GAME_INGAMEICONZ_POWERUPZ_CONVERSION";
            break;
        case 58:
            name = "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH";
            break;
        case 54:
            name = "GAME_INGAMEICONZ_POWERUPZ_GHOST";
            break;
        case 60:
            name = "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR";
            break;
        case 59:
            name = "GAME_INGAMEICONZ_POWERUPZ_ROIDZ";
            break;
        case 56:
            name = "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY";
            break;
        case 55:
            name = "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED";
            break;
        case 90:
            name = "GAME_INGAMEICONZ_SECRETW";
            break;
        case 91:
            name = "GAME_INGAMEICONZ_SECRETA";
            break;
        case 92:
            name = "GAME_INGAMEICONZ_SECRETR";
            break;
        case 93:
            name = "GAME_INGAMEICONZ_SECRETP";
            break;
        case 75:
            name = "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH";
            break;
        case 80:
            name = "GAME_INGAMEICONZ_POWERUPZ_COIN";
            break;
        case 99: {
            CIconSprite* tb = g_gameReg->m_factoryHolder->m_factory
                                  ->CreateSprite(0, geoB, geoA, 0xf, "TimeBomb", 0x40003);
            if (tb) {
                tb->m_120 = g_buteMgr.GetDwordDef("Powerupz", "CoveredTimeBombTime", 0x7d0);
            }
            return tb != 0;
        }
        default:
            return 0;
    }

    CIconSprite* spr = g_gameReg->m_factoryHolder->m_factory
                           ->CreateSprite(0, geoB, geoA, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        return 0;
    }
    spr->CacheFirstFrame(name);
    spr->m_120 = m120;
    spr->m_114 = 0;
    spr->m_118 = 0;
    spr->m_124 = 0;
    spr->m_11c = 0;
    spr->m_128 = 0;
    spr->m_12c = 0;
    spr->m_130 = m130;
    return 1;
}

SIZE_UNKNOWN(CResourceTracker);
SIZE_UNKNOWN(CSpriteVtbl);
SIZE_UNKNOWN(EngineLabelBacklog);
