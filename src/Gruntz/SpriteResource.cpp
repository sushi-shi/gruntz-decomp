#include <rva.h>
// SpriteResource.cpp - the shared engine sprite-resource helpers that the HUD /
// powerup / explosion / camera / status-bar loaders all funnel through
// (C:\Proj\Gruntz). Each pulls a named sprite-set out of the engine's
// string-keyed sprite-set hash table (CSpriteHashTable::Lookup, external,
// reloc-masked) and then caches/forwards something off the looked-up sprite.
//
// These are the small shared LEAVES that block dozens of downstream loaders:
//
//   CGruntSprite::CacheFirstFrame  @0x150540 (this = a created sprite; reached as
//       this->m_c->m_10->map; caches the sprite's FIRST valid frame:
//       m_194 = the sprite, m_190 = the first frame number N (=spr->m_64),
//       m_198 = spr->m_14[N] only if N is in the inclusive [m_64..m_68] range).
//       1-arg __thiscall (ret 4). Called by LoadCameraSprite right after the
//       factory builds the sprite + runs its init virtual.
//
//   CGruntSprite::ApplyGeometry    @0x1505b0 (this = a created sprite; reached as
//       this->m_c->m_2c->map; on a successful lookup runs the geometry setter on
//       the sub-player @this+0x1a0, then - if the global default source is set -
//       a second geometry-source apply). 2-arg __thiscall (ret 8).
//
//   CSpriteFactory::CreateSprite   @0x1597b0 (this = the global HUD sprite factory
//       reached via g->m_30->m_8; reached as this->m_c->m_14->map). Looks the
//       sprite-template up by class-NAME string (the 5th arg), then forwards the
//       remaining 5 build args + the template to the real allocator/ctor
//       CSpriteFactory::CreateSpriteImpl @0x159600 (external). 6-arg __thiscall
//       (ret 0x18). Returns the created sprite (or 0 if the template is missing).
//
// Only offsets / code bytes are load-bearing; names are placeholders for the
// engine identities recovered from the call sites.

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table + the sprite object, modeled
// minimally (the same shape SpriteLoaders.cpp uses): Lookup() hashes the
// class-name key and writes the found sprite pointer through *ppOut. The
// `add ecx,0x10; call <lookup>` reloc-masks against the matched lookup helper.
// ---------------------------------------------------------------------------
struct CSprite;
class CSpriteHashTable {
public:
    i32 Lookup(const char* szName, CSprite** ppOut);
};

struct CSprite {
    char m_pad00[0x14];
    i32** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  first valid frame number
    i32 m_68; // +0x68  last valid frame number
};

// The sprite-set manager: the name->sprite hash table is embedded at +0x10.
struct CSpriteMgr {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
};

// The resource object the sprite reaches the sprite manager through. Different
// loaders pull the manager out of a different slot (+0x10 / +0x14 / +0x2c),
// so each helper names the slot it uses.
struct CResMgr {
    char m_pad00[0x10];
    CSpriteMgr* m_10; // +0x10  (CacheFirstFrame)
    CSpriteMgr* m_14; // +0x14  (CreateSprite)
    char m_pad18[0x2c - 0x18];
    CSpriteMgr* m_2c; // +0x2c  (ApplyGeometry)
};

// ===========================================================================
// CGruntSprite::CacheFirstFrame  @0x150540
// ===========================================================================
//
// this->m_c is the resource object; the sprite-set manager is at m_c->m_10; the
// looked-up sprite's FIRST frame number (spr->m_64) indexes the frame table.
// The range guard is the generic `(m_64 <= N && N <= m_68)` shape with N=m_64,
// so the first compare is m_64 vs m_64 (always equal); it is written verbatim so
// MSVC emits both reads.
class CGruntSprite {
public:
    void CacheFirstFrame(const char* name);

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
    char m_pad10[0x190 - 0x10];
    i32 m_190;      // +0x190  first frame number
    CSprite* m_194; // +0x194  the looked-up sprite
    i32* m_198;     // +0x198  the first frame's pointer (or 0)
};

RVA(0x00150540, 0x65)
void CGruntSprite::CacheFirstFrame(const char* name) {
    CSprite* spr = 0;
    m_c->m_10->m_10map.Lookup(name, &spr);
    m_194 = spr;
    if (spr) {
        i32 n = spr->m_64;
        m_190 = n;
        if (n >= spr->m_64 && n <= spr->m_68) {
            m_198 = spr->m_14[n];
            return;
        }
    }
    m_198 = 0;
}

// ===========================================================================
// CSpriteFactory::CreateSprite  @0x1597b0
// ===========================================================================
//
// The public sprite-creation entry the HUD / level loaders call (reached via the
// global game registry -> +0x30 -> +0x8). It looks the sprite TEMPLATE up by its
// class-NAME string (the `name` arg) in the factory's sprite-set table
// (this->m_c->m_14->map), and on a hit forwards the four leading build args + the
// resolved template + the trailing flags arg to the real allocator/constructor
// CreateSpriteImpl @0x159600 (external; it `new`s 0x1dc bytes under an EH frame
// and constructs the named sprite). On a miss it returns 0. __thiscall, ret 0x18.
//
// The `name` (lookup key) is consumed by the lookup and is NOT forwarded to the
// impl - the impl receives the resolved template pointer in its place.

class CSpriteFactory {
public:
    CSprite* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // The real allocator/ctor (external/no-body so the call reloc-masks).
    CSprite* CreateSpriteImpl(i32 kind, i32 geoB, i32 geoA, i32 hint, CSprite* tmpl, i32 flags);

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
};

RVA(0x001597b0, 0x57)
CSprite*
CSpriteFactory::CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags) {
    CSprite* tmpl = 0;
    m_c->m_14->m_10map.Lookup(name, &tmpl);
    if (!tmpl) {
        return 0;
    }
    return CreateSpriteImpl(kind, geoB, geoA, hint, tmpl, flags);
}

// ===========================================================================
// CStatusBarMgr::ConfigureItem  @0x1360d0
// ===========================================================================
//
// The shared status-bar item-configuration helper the per-widget status-bar
// loaders (UpdateGruntOvenStatusBar, UpdateWarpStoneStatusBar,
// LoadSwitchUp/DownSprite, LoadStatzTabToggleSprite, UpdateChipGrinderStatusBar,
// ...) funnel through. It guards on the status-bar surface being live
// (this->m_10->m_78 != 0), then resolves the backing status-bar ITEM via
// GetItem @0x135d70 (__thiscall on this) and pushes four configuration values
// into it through four __thiscall setters @0x1355c0/0x1357a0/0x135920/0x135510
// (each ret 4) plus a finalize @0x136270 (ret 0). The first three setters and the
// finalizer each AND their success flag into the accumulator (the 4th setter's
// result is ignored). __thiscall, ret 0x10 = 4 stack args. Returns the
// success flag (1 only if every checked step succeeded).

struct CStatusBarSurface {
    char m_pad00[0x78];
    i32 m_78; // +0x78  the live-surface gate
};

class CStatusBarItem2 {
public:
    i32 SetField0(i32 v); // 0x1355c0  (ret 4)
    i32 SetField1(i32 v); // 0x1357a0  (ret 4)
    i32 SetField2(i32 v); // 0x135920  (ret 4)
    i32 SetField3(i32 v); // 0x135510  (ret 4, result ignored)
    i32 Finalize();       // 0x136270  (ret 0)
};

class CStatusBarMgr {
public:
    i32 ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3);
    CStatusBarItem2* GetItem(); // 0x135d70  (ret 0)

    char m_pad00[0x10];
    CStatusBarSurface* m_10; // +0x10
};

RVA(0x001360d0, 0x7c)
i32 CStatusBarMgr::ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (!m_10->m_78) {
        return 0;
    }
    CStatusBarItem2* item = GetItem();
    if (!item) {
        return 0;
    }
    i32 ok = 1;
    if (!item->SetField0(a0)) {
        ok = 0;
    }
    if (!item->SetField1(a1)) {
        ok = 0;
    }
    if (!item->SetField2(a2)) {
        ok = 0;
    }
    item->SetField3(a3);
    if (!item->Finalize()) {
        ok = 0;
    }
    return ok;
}

// ===========================================================================
// CGruntAnimPlayer::ApplyLookupGeometry  @0x1505b0
// ===========================================================================
//
// A second sprite-resource resolver on the per-grunt animation player (the
// CGrunt+0x38 player; callers do `m_40 = m_38->m_1b4` then call this). It looks a
// named sprite-set up through this->m_c->m_2c->map and, on a hit, drives the
// geometry sub-player @this+0x1a0: SetGeometry(spr) @0x15c2d0 (ret 4); then, when
// the second arg is set, applies the global default geometry source g_defaultGeo
// (@0x6bf3bc) via a sibling setter @0x15c360 (ret 4). Returns 1 on a hit, 0 on a
// miss. __thiscall, ret 8 = 2 stack args.

// The geometry sub-player @player+0x1a0 (engine; both setters are __thiscall ret 4).
class CGruntAnimSub2 {
public:
    void SetGeometry(i32 srcSprite); // 0x15c2d0
    void SetGeoSource(i32 src);      // 0x15c360
};

// The global default geometry source the second setter consumes.
DATA(0x002bf3bc)
extern i32 g_defaultGeo; // VA 0x6bf3bc (RVA 0x2bf3bc)

class CGruntAnimPlayer {
public:
    i32 ApplyLookupGeometry(const char* name, i32 applyDefault);

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
    char m_pad10[0x1a0 - 0x10];
    CGruntAnimSub2 m_1a0; // +0x1a0  geometry sub-player
};

RVA(0x001505b0, 0x5c)
i32 CGruntAnimPlayer::ApplyLookupGeometry(const char* name, i32 applyDefault) {
    CSprite* spr = 0;
    m_c->m_2c->m_10map.Lookup(name, &spr);
    if (!spr) {
        return 0;
    }
    m_1a0.SetGeometry((i32)spr);
    if (applyDefault) {
        m_1a0.SetGeoSource(g_defaultGeo);
    }
    return 1;
}
