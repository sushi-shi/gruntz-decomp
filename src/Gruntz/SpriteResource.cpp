#include <rva.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/ResMgr.h>        // CResMgr + the three registries (m_10/m_14/m_28/m_2c)
#include <Gruntz/Sprite.h>        // CSprite (frame-data), CSpriteHashTable, CFrameArray
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory shape (this TU owns CreateSprite@0x1597b0)
#include <Gruntz/SoundCueMgr.h> // the ONE CSoundCueMgr shape (this TU owns ConfigureItem@0x1360d0)
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
//       m_sprite = the sprite, m_frameNum = the first frame number N (=spr->m_64),
//       m_framePtr = spr->m_14[N] only if N is in the inclusive [m_64..m_68] range).
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

// CSpriteHashTable / CSprite (frame-data value) / CFrameArray now live in the
// shared <Gruntz/Sprite.h>; CResMgr + its registries in <Gruntz/ResMgr.h>.

// The global NAFXCW allocator the inlined frame-worker construction uses.
extern void* operator new(u32 size);

// ---------------------------------------------------------------------------
// The frame-worker is a CImage (RTTI .?AVCImage@@, SHARED vtable ??_7CImage@@6B@
// @0x1eaa2c / VA 0x5eaa2c, cataloged in config/vtable_names.csv). The insert
// allocates a raw 0x34-byte CImage and INLINES its construction (vptr stamp + field
// init) at the new-site - it does NOT call an out-of-line ctor - then drives the
// slot-11 Resolve virtual and, on failure, the slot-1 scalar dtor. Now modeled
// real-polymorphic (all-vtables mandate): cl auto-stamps the vptr
// (??_7CFrameWorker@@6B@; the foreign CImage slots stay declared-only, reloc-masked)
// so both dispatches lower to `mov eax,[this]; call [eax+off]`. Slots named by their
// retail vtable-slot RVA read from the 0x1eaa2c .rdata (FUN_<rva>); the low ones
// (0x1000-0x7c20) are ILT jmp-thunks into the CObject/MFC base. NO VTBL(): the vtable
// is the shared ??_7CImage. Manual CFrameWorkerVtbl PMF table + g_imageVtbl stamp
// removed.
// ---------------------------------------------------------------------------
struct CFrameWorker {
    virtual void GetRuntimeClass();               // [0]  +0x00
    virtual void FUN_00002adb(i32 flag);          // [1]  +0x04  scalar-deleting dtor (ILT)
    virtual void Serialize();                     // [2]  +0x08 (ILT)
    virtual void AssertValid();                   // [3]  +0x0c (ILT)
    virtual void Dump();                          // [4]  +0x10 (ILT)
    virtual void FUN_000013b6();                  // [5]  +0x14 (ILT)
    virtual void FUN_00001c08();                  // [6]  +0x18 (ILT)
    virtual void FUN_00153260();                  // [7]  +0x1c  CImage::FreeAll
    virtual void FUN_000042aa();                  // [8]  +0x20 (ILT)
    virtual void FUN_001530e0();                  // [9]  +0x24  CImage::Create24
    virtual void FUN_00152fb0();                  // [10] +0x28  CImage::LoadDispatch
    virtual i32 FUN_00152f20(void* src, i32 arg); // [11] +0x2c  CImage::Resolve

    inline CFrameWorker(i32 frameNumber, void* parent) {
        m_04 = frameNumber;
        m_08 = 0;
        m_0c = parent;
        m_10 = 0;
        m_14 = 0;
        m_2c = 0;
        m_30 = 0;
    }

    i32 m_04;              // +0x04  frame number
    i32 m_08;              // +0x08
    void* m_0c;            // +0x0c  parent (sprite->m_c)
    i32 m_10;              // +0x10  (zeroed)
    i32 m_14;              // +0x14  (zeroed)
    char _18[0x2c - 0x18]; // +0x18  (untouched)
    i32 m_2c;              // +0x2c
    i32 m_30;              // +0x30
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
//
// DUAL-MODEL NOTE (resolved: REAL SPLIT, not a fold): `CGruntSprite` here is the
// frame-cache BOUND game-object B - the receiver of these 0x1504d0/0x150540
// leaves (m_c @ +0xc, cache slots @ +0x190/0x194/0x198). It is NOT the CGrunt*Sprite
// HUD family (CGruntStaminaSprite/ToyTimeSprite/WingzTimeSprite : CGruntHealthSprite,
// ctors in GameObjectCtors.cpp). Those HUD sprites BIND B: their m_10 == m_38 ==
// the ctor arg `obj` == B, and call B->ApplyLookupSprite (== this CacheFrame). So A
// (HUD sprite) and B (this cache object) are DISTINCT objects - a real split. B is
// the engine's bound game object (GameObjectCtors names the same B `CSpriteObj`; its
// true identity is the shared CGameObject, of which `CGruntSprite`/`CGruntAnimPlayer`/
// `CSpriteObj` are placeholder field-views); folding all of B onto CGameObject is
// deferred (it would touch several @early-stop leaves here) and is orthogonal to the
// HUD-family split resolved above.
class CGruntSprite {
public:
    void CacheFirstFrame(const char* name);
    void CacheFrame(const char* name, i32 frame);

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
    char m_pad10[0x190 - 0x10];
    i32 m_frameNum;    // +0x190  first frame number
    CSprite* m_sprite; // +0x194  the looked-up sprite
    i32* m_framePtr;   // +0x198  the first frame's pointer (or 0)
};

RVA(0x00150540, 0x65)
void CGruntSprite::CacheFirstFrame(const char* name) {
    CSprite* spr = 0;
    m_c->m_10->m_10map.Lookup(name, &spr);
    m_sprite = spr;
    if (spr) {
        i32 n = spr->m_firstFrame;
        m_frameNum = n;
        if (n >= spr->m_firstFrame && n <= spr->m_lastFrame) {
            m_framePtr = spr->m_frames.m_pData[n];
            return;
        }
    }
    m_framePtr = 0;
}

// ===========================================================================
// CGruntSprite::CacheFrame  @0x1504d0
// ===========================================================================
//
// As CacheFirstFrame, but the frame number is supplied by the caller (arg 2)
// instead of taken from spr->m_64. Looks the named sprite up through m_c->m_10,
// caches it at m_sprite, and - on a hit and a frame in [m_64..m_68] - caches the
// frame's pointer at m_framePtr and the frame number at m_frameNum; otherwise m_frameNum still
// records the requested frame and m_framePtr is 0. __thiscall, ret 8.
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// the `mov [&spr],0` sinks past the arg pushes + the extra frame arg flips the
// sprite/frame eax<->ecx allocation; identical instruction multiset, ~84%. Same
// wall as the sibling CacheFirstFrame (89%). Logic complete.
RVA(0x001504d0, 0x6c)
void CGruntSprite::CacheFrame(const char* name, i32 frame) {
    CSprite* spr = 0;
    m_c->m_10->m_10map.Lookup(name, &spr);
    m_sprite = spr;
    if (spr) {
        if (frame >= spr->m_firstFrame && frame <= spr->m_lastFrame) {
            m_frameNum = frame;
            m_framePtr = spr->m_frames.m_pData[frame];
        } else {
            m_frameNum = frame;
            m_framePtr = 0;
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

// The sprite object the factory installs/initialises (AttachSprite's arg0). Its
// init virtual lives at vtable slot 0x28 (a 4-arg method returning a success flag);
// its +0x7c sub-table holds a plain fn-ptr entry at +0x10 driven post-attach.
// __single_inheritance keeps the init PMF a 4-byte code pointer (the class is only
// forward-declared here, else MSVC's general PMF emits a this-adjust thunk).
// CSprite2 is forward-declared (with __single_inheritance) in <Gruntz/SpriteFactory.h>.
typedef i32 (CSprite2::*CSprite2InitFn)(i32, i32, i32, CSprite*);
struct CSprite2Vtbl {
    char _00[0x28];
    CSprite2InitFn Init; // [0x28]
};
// The +0x7c fn-ptr sub-table: the post-attach driver entry lives at byte +0x10.
struct CSprite2SubTable {
    void* _00[4];             // +0x00..0x0c
    void (*Entry)(CSprite2*); // +0x10  post-attach driver
};
struct CSprite2 {
    CSprite2Vtbl* vptr; // +0x00
    char _04[0x08 - 0x04];
    i32 m_08; // +0x08  flags slot
    char _0c[0x7c - 0x0c];
    CSprite2SubTable* m_7c; // +0x7c  fn-ptr sub-table (entry @+0x10)

    i32 Init(i32 a, i32 b, i32 c, CSprite* tmpl) {
        return (this->*(vptr->Init))(a, b, c, tmpl);
    }
};

// CSpriteFactory now lives in the shared <Gruntz/SpriteFactory.h> (included above);
// this TU owns CreateSprite (0x1597b0) + AttachSprite (0x159830). The lookup RESULT is
// a CSprite frame-data template; the created INSTANCE is the shared CGameObject.
RVA(0x001597b0, 0x57)
CGameObject*
CSpriteFactory::CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags) {
    CSprite* tmpl = 0;
    m_c->m_14->m_10map.Lookup(name, &tmpl);
    if (!tmpl) {
        return 0;
    }
    return CreateSpriteImpl(kind, geoB, geoA, hint, tmpl, flags);
}

// ===========================================================================
// CSpriteFactory::AttachSprite  @0x159830
// ===========================================================================
//
// Initialise an already-allocated sprite (arg0) against a named template: look the
// template up by class-NAME (arg4) in the factory's sprite-set table; on a miss
// return 0. On a hit stamp the flags arg into the sprite (+0x8), run its init
// virtual (vtable slot 0x28) with (a1, a2, a3, template) and bail with 0 if it
// fails; otherwise splice the sprite into the factory's child list (AddChild) and,
// when the flags carry 0x200000, drive the sprite's +0x7c sub-table entry @+0x10.
// Returns 1 on success. __thiscall, ret 0x18.
// @early-stop
// 99.5% ebx<->edi coloring wall: byte-identical except retail colors this->edi /
// flags->ebx while MSVC5 picks this->ebx / flags->edi (6 reg-only instr diffs); the
// instruction selection (PMF init call, m_7c dispatch) is exact, not source-steerable.
RVA(0x00159830, 0x92)
i32 CSpriteFactory::AttachSprite(
    CSprite2* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    const char* name,
    i32 flags
) {
    if (!obj) {
        return 0;
    }
    CSprite* tmpl = 0;
    m_c->m_14->m_10map.Lookup(name, &tmpl);
    if (!tmpl) {
        return 0;
    }
    obj->m_08 = flags;
    if (!obj->Init(a1, a2, a3, tmpl)) {
        return 0;
    }
    AddChild(obj, 1);
    if (flags & 0x200000) {
        obj->m_7c->Entry(obj);
    }
    return 1;
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

// CStatusBarSurface / CStatusBarItem2 / CSoundCueMgr now live in the shared
// <Gruntz/SoundCueMgr.h> (included above); this TU owns ConfigureItem (0x1360d0).
RVA(0x001360d0, 0x7c)
i32 CSoundCueMgr::ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (!m_10->m_78) {
        return 0;
    }
    CStatusBarItem2* item = GetItem();
    if (!item) {
        return 0;
    }
    i32 ok = 1;
    if (!((DirectSoundMgr*)item)->SetVolumeByIndex(a0)) {
        ok = 0;
    }
    if (!((DirectSoundMgr*)item)->SetPanByIndex(a1)) {
        ok = 0;
    }
    if (!((DirectSoundMgr*)item)->SetField2(a2)) {
        ok = 0;
    }
    ((DirectSoundMgr*)item)->SetField3(a3);
    if (!((DirectSoundMgr*)item)->Play()) {
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
    i32 LookupAnimSprite(const char* name); // 0x150610

    char m_pad00[0xc];
    CResMgr* m_c; // +0x0c
    char m_pad10[0x19c - 0x10];
    CSprite* m_19c;       // +0x19c  cached looked-up sprite
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
// CGruntAnimPlayer::LookupAnimSprite  @0x150610
// ===========================================================================
//
// Looks the named sprite-set up through this->m_c->m_28->map; on a hit caches the
// resolved sprite at this+0x19c and returns 1, else returns 0. __thiscall, ret 4.
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// identical instruction multiset to the sibling Cache* lookups (CacheFrame 84% /
// CacheFirstFrame 89%); the `mov [&spr],0` sinks past the arg pushes. ~73%; logic
// complete, deferred to the final sweep.
RVA(0x00150610, 0x41)
i32 CGruntAnimPlayer::LookupAnimSprite(const char* name) {
    CSprite* spr = 0;
    m_c->m_28->m_10map.Lookup(name, &spr);
    if (spr != 0) {
        m_19c = spr;
        return 1;
    }
    return 0;
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
    if (n < m_frames.m_nSize && m_frames.m_pData[n] != 0) {
        return 0;
    }
    CFrameWorker* worker = new CFrameWorker(n, m_c);
    if (!worker->FUN_00152f20(src, mode)) { // slot 11 @+0x2c  CImage::Resolve
        if (worker) {
            worker->FUN_00002adb(1); // slot 1 @+0x04  scalar-deleting dtor
        }
        return 0;
    }
    m_frames.SetAtGrow(n, (CObject*)worker);
    if (n < m_firstFrame) {
        m_firstFrame = n;
    }
    if (n > m_lastFrame) {
        m_lastFrame = n;
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
    if (n >= m_firstFrame && n <= m_lastFrame) {
        return (i32)m_frames.m_pData[n];
    }
    return 0;
}
// CFrameWorker is real-polymorphic (cl emits ??_7CFrameWorker). Its vtable is the
// SHARED ??_7CImage@@6B@ (0x1eaa2c) - no per-class VTBL (would collide/misname). Exact
// size 0x34: the insert allocates the 0x34-byte raw CImage (ctor writes to m_30+0x30).
SIZE(CFrameWorker, 0x34);
SIZE_UNKNOWN(CGruntAnimPlayer);
SIZE_UNKNOWN(CGruntAnimSub2);
SIZE_UNKNOWN(CGruntSprite);
SIZE_UNKNOWN(CSprite2);
SIZE_UNKNOWN(CSprite2SubTable);
SIZE_UNKNOWN(CSprite2Vtbl);
