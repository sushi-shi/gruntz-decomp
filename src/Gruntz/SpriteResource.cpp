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

// ---------------------------------------------------------------------------
// The CSprite frame table is a CObArray of CImage frame-workers living AT
// this+0x10 (so its m_pData is this+0x14 = m_14, its m_nSize this+0x18). The
// frame-worker INSERT (0x151f00) grows this array via SetAtGrow and the frame
// READ (0x15cc30) indexes m_pData directly. Modeling the array at +0x10 is
// matching-neutral (it does not move m_14/m_64/m_68).
//
// SetAtGrow(index, element): grows the backing store to fit `index`, then stores
// element at [index]. __thiscall ret 8, no body so the call reloc-masks against
// the engine CObArray helper @0x1b5822.
// ---------------------------------------------------------------------------
struct CObject;
struct CFrameArray {
    void* m_vptr;     // +0x00  CObject vftable
    i32** m_pData;    // +0x04  frame-pointer table
    i32 m_nSize;      // +0x08  element count
    i32 m_nMaxSize;   // +0x0c
    i32 m_nGrowBy;    // +0x10
    void SetAtGrow(i32 index, CObject* element); // 0x1b5822
};

// The global NAFXCW allocator the inlined frame-worker construction uses.
extern void* operator new(u32 size);

// ---------------------------------------------------------------------------
// The frame-worker is a CImage (RTTI .?AVCImage@@, primary vftable @0x5eaa2c).
// The insert allocates a raw 0x34-byte CImage and INLINES its construction
// (vptr stamp + field init) at the new-site - it does NOT call an out-of-line
// ctor - then drives the slot-11 Resolve virtual and, on failure, the slot-1
// scalar dtor. Both are manual vtable dispatches because the vptr is a raw
// reloc-masked stamp (the table contents are external engine code).
// ---------------------------------------------------------------------------
DATA(0x001eaa2c)
extern void* g_imageVtbl; // 0x5eaa2c - CImage primary vftable

struct CFrameWorkerVtbl;
struct CFrameWorker {
    CFrameWorkerVtbl* vptr; // +0x00
    i32 m_04;               // +0x04  frame number
    i32 m_08;               // +0x08
    void* m_0c;             // +0x0c  parent (sprite->m_c)
    char _10[0x2c - 0x10];  // +0x10  (m_10/m_14 zeroed, rest untouched)
    i32 m_2c;               // +0x2c
    i32 m_30;               // +0x30

    i32 Resolve(void* src, i32 arg); // vtbl[0x2c] (slot 11)
    void Destroy(i32 flag);          // vtbl[0x04] (slot 1, scalar dtor)
};
typedef i32 (CFrameWorker::*ResolveFn)(void*, i32);
typedef void (CFrameWorker::*DestroyFn)(i32);
struct CFrameWorkerVtbl {
    DestroyFn Destroy;     // [0x04]
    char _08[0x2c - 0x08]; // slots 2..10
    ResolveFn Resolve;     // [0x2c]
};
inline i32 CFrameWorker::Resolve(void* src, i32 arg) {
    return (this->*(vptr->Resolve))(src, arg);
}
inline void CFrameWorker::Destroy(i32 flag) {
    (this->*(vptr->Destroy))(flag);
}

struct CSprite {
    char m_pad00[0xc];
    void* m_c;          // +0x0c  parent context handed to each frame worker
    CFrameArray m_10;   // +0x10  frame CObArray (m_pData @+0x14, m_nSize @+0x18)
    char m_pad24[0x64 - 0x24];
    i32 m_64; // +0x64  first valid frame number
    i32 m_68; // +0x68  last valid frame number

    // Insert a frame worker at frame number `n` (0x151f00); read a frame (0x15cc30).
    CFrameWorker* InsertFrame(void* src, i32 n, i32 mode);
    i32 GetFrame(i32 n);
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
    void CacheFrame(const char* name, i32 frame);

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
            m_198 = spr->m_10.m_pData[n];
            return;
        }
    }
    m_198 = 0;
}

// ===========================================================================
// CGruntSprite::CacheFrame  @0x1504d0
// ===========================================================================
//
// As CacheFirstFrame, but the frame number is supplied by the caller (arg 2)
// instead of taken from spr->m_64. Looks the named sprite up through m_c->m_10,
// caches it at m_194, and - on a hit and a frame in [m_64..m_68] - caches the
// frame's pointer at m_198 and the frame number at m_190; otherwise m_190 still
// records the requested frame and m_198 is 0. __thiscall, ret 8.
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// the `mov [&spr],0` sinks past the arg pushes + the extra frame arg flips the
// sprite/frame eax<->ecx allocation; identical instruction multiset, ~84%. Same
// wall as the sibling CacheFirstFrame (89%). Logic complete.
RVA(0x001504d0, 0x6c)
void CGruntSprite::CacheFrame(const char* name, i32 frame) {
    CSprite* spr = 0;
    m_c->m_10->m_10map.Lookup(name, &spr);
    m_194 = spr;
    if (spr) {
        if (frame >= spr->m_64 && frame <= spr->m_68) {
            m_190 = frame;
            m_198 = spr->m_10.m_pData[frame];
        } else {
            m_190 = frame;
            m_198 = 0;
        }
    }
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
    void ApplyGeometryDirect(i32 srcSprite, i32 applyDefault);

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

// ===========================================================================
// CGruntAnimPlayer::ApplyGeometryDirect  @0x58b60
// ===========================================================================
//
// The direct counterpart of ApplyLookupGeometry: the sprite source is passed in
// directly (no name lookup), so it ALWAYS drives the +0x1a0 geometry sub-player's
// SetGeometry(srcSprite), then - when the second arg is set - applies the global
// default geometry source g_defaultGeo via the sibling setter. __thiscall, ret 8.
RVA(0x00058b60, 0x2d)
void CGruntAnimPlayer::ApplyGeometryDirect(i32 srcSprite, i32 applyDefault) {
    m_1a0.SetGeometry(srcSprite);
    if (applyDefault) {
        m_1a0.SetGeoSource(g_defaultGeo);
    }
}

// ===========================================================================
// CSprite::InsertFrame  @0x151f00
// ===========================================================================
//
// Build and install a frame worker (a CImage, vftable @0x5eaa2c) at frame number
// `n` in the sprite's +0x10 frame CObArray. If the slot is already occupied
// (n < m_nSize && m_pData[n] != 0) it bails with 0 (no overwrite). Otherwise it
// allocates a raw 0x34-byte CImage and INLINES the construction at the new-site
// (vptr stamp + frame number into m_04, parent into m_0c, zero m_08/m_10/m_14/
// m_2c/m_30), drives the slot-11 Resolve virtual with (src, mode); on failure it
// runs the worker's slot-1 scalar dtor (delete=1) and returns 0. On success it
// SetAtGrow's the worker into the array at `n`, widens the [m_64..m_68] valid
// frame range, and returns the worker. __thiscall, ret 0xc.
// @early-stop
// regalloc wall (pin-local-for-callee-saved-reg.md): the body is byte-identical
// from the operator-new site (off 0x22) to the end, but retail colors the
// callee-saved triple this->esi / n->edi / worker->ebx (loading n from the stack
// late, post-prologue) while MSVC5 here colors this->edi / n->ebx / worker->esi
// (n loaded eagerly). The rotation is the entry coloring, not source-steerable;
// flipping the guard operands didn't move it. ~84%, logic complete.
RVA(0x00151f00, 0xa4)
CFrameWorker* CSprite::InsertFrame(void* src, i32 n, i32 mode) {
    if (n < m_10.m_nSize && m_10.m_pData[n] != 0) {
        return 0;
    }
    CFrameWorker* worker = (CFrameWorker*)operator new(0x34);
    if (worker) {
        worker->m_04 = n;
        worker->m_08 = 0;
        worker->m_0c = m_c;
        worker->vptr = (CFrameWorkerVtbl*)&g_imageVtbl;
        *(i32*)(worker->_10 + 0x00) = 0; // +0x10
        *(i32*)(worker->_10 + 0x04) = 0; // +0x14
        worker->m_2c = 0;
        worker->m_30 = 0;
    } else {
        worker = 0;
    }
    if (!worker->Resolve(src, mode)) {
        if (worker) {
            worker->Destroy(1);
        }
        return 0;
    }
    m_10.SetAtGrow(n, (CObject*)worker);
    if (n < m_64) {
        m_64 = n;
    }
    if (n > m_68) {
        m_68 = n;
    }
    return worker;
}

// ===========================================================================
// CSprite::GetFrame  @0x15cc30
// ===========================================================================
//
// Bounds-checked frame read: if `n` is in the inclusive valid range
// [m_64..m_68], return the frame pointer m_pData[n] (as an int); else 0. The
// same shape as CacheFirstFrame's lookup, but as a standalone __thiscall (ret 4).
RVA(0x0015cc30, 0x1e)
i32 CSprite::GetFrame(i32 n) {
    if (n >= m_64 && n <= m_68) {
        return (i32)m_10.m_pData[n];
    }
    return 0;
}
