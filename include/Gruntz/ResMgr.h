// ResMgr.h - the ONE shape for the game resource/level manager (CResMgr) and the
// three named-object registries it owns. Previously ActionOptionsMenuBar.cpp,
// SpriteResource.cpp, SpriteLoaders.cpp and CPlay.cpp each carried a divergent
// partial view of the same retail CResMgr; this is the single reconstructed layout.
//
// CResMgr holds three DISTINCT registry classes (their Has()/Register()/Install()
// helpers sit at different retail addresses, so they are genuinely different
// types, not one generic manager): the image/tile registry at +0x10, the sound
// registry at +0x28 and the animation registry at +0x2c. Each registry embeds the
// same name->object hash table at ITS +0x10 (m_10map). The sprite/HUD loaders
// reach a sprite through <registry>->m_10map.Lookup(); the level loader (CPlay)
// drives the registries' install helpers.
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_RESMGR_H
#define GRUNTZ_RESMGR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Sprite.h> // CSpriteHashTable (each registry's +0x10 map)

struct CMenuViewObj; // the level/view object at CResMgr+0x24 (ActionOptionsMenuBar-local)
struct CGMInputObj;  // CDrawTarget::SurfaceA::Surface2c::m_8 (per-frame input obj; GameMode.h)
class CDDSurface;    // CDrawTarget::SurfaceB::m_2c (the real DDraw view surface; DDSurface.h)

// The pooled resource the leaf states free before releasing their namespaces
// (m_28->m_2c). Reloc-masked __thiscall. (Was View.h's CViewPooledRes.)
SIZE_UNKNOWN(CViewPooledRes);
struct CViewPooledRes {
    // Free @? IS SoundStream::Stop; cast at each call.
    // TickAnim @? IS SoundDevice::PurgeVoiceList; cast at the call.
};

// The active draw surface / render-flip pump at CResMgr+0x04. The loaders read its
// +0x14 as an opaque draw-context handle; the render/credits path drives it as the flip
// pump: Flush (0x158ee0 == CDDrawWorkerMgr::Method_158ee0 - the state activators call the
// SAME fn as the resource worker-apply on activate), the frame-surface page (+0x10), the
// draw-surface view (+0x14, the same handle the loaders read as m_14) and the present
// target (+0x18). (This is the real +0x04 class; the former View.h StateMgrBZ view folds
// here.) The nested surface pages are DDraw objects; their leaf +0x2c is the real CDDSurface.
SIZE_UNKNOWN(CDrawTarget);
struct CDrawTarget {
    char m_pad00[0x10];
    struct SurfaceA { // +0x10  frame-surface page
        char p0[0x2c];
        struct Surface2c {    // +0x2c the frame surface (CPlay casts it to CProfFlush)
            void Flip(i32 z); // FUN_0013e850 (resource-facet flip, ret 4)
            void Draw(i32 z); // credits draw-target draw (thiscall)
            char p0[0x8];
            CGMInputObj* m_8; // +0x08  input obj (credits input poll source)
        }* m_2c;
    }* m_10;
    struct SurfaceB {       // +0x14  draw-surface view (the loaders' "draw context" handle)
        void Blit(i32 arg); // credits blit-target blit (thiscall)
        char p0[0x2c];
        CDDSurface* m_2c; // +0x2c  the real CDDSurface (Fill @0x13e760 / Restore @0x13e7d0)
    }* m_14;
    void* m_18; // +0x18  the present target
};

// The world/key lookup table at CResMgr+0x08 the timer-expiry path probes
// (FindByKey / engine "Lookup" @0x1b8760, reached at +0x48). Modeled NO-body.
SIZE_UNKNOWN(CKeyTable);
struct CKeyTable {
    i32 FindByKey(i32 key, i32* outFound);
};

// The image/tile registry at CResMgr+0x10: a virtual Install at vtable slot 18
// (+0x48) plus non-virtual Has/Register helpers, and the name->sprite hash table
// embedded at its own +0x10. All methods external/no-body so the calls reloc-mask.
//
// This is the ONE image/name registry class - the render/resource state TUs reach it as
// CState::m_c->m_10 (the former View.h CSpriteFactoryHolder::CImageRegistry view folds here); the
// int-keyed frame-grid Lookup at CPlay::BeginGridWalk casts to the name-keyed m_10map.Lookup.
// FOREIGN object - the honest form (manual m_vtbl into a typed CImageRegistryVtbl
// naming only the +0x48 Install slot + char pad) is WALLED here: converting this
// polymorphic class to the non-polymorphic PMF form flips CSBI_MenuItem::DecCounter
// (sbi_menuitem, RVA 0xe82a0) 100->92.04% - a whole-TU load-schedule coin-flip (that
// fn's own @early-stop notes it is a non-steerable 1-instruction schedule swap). The
// drop is identical (92.0357) whether the Vtbl is inlined or moved to end-of-header, so
// it is not layout-steerable. Left in the vNN placeholder form to preserve DecCounter's
// 100%; re-attack in the final sweep once DecCounter is stably matched.
SIZE_UNKNOWN(CImageRegistry);
struct CImageRegistry {
    // Has @0x155550 IS CDDrawWorkerRegistry::HasKeyEqual_155550; cast at each call.
    // Register @0x155360 IS CDDrawWorkerRegistry::RemoveKeysEqual_155360; cast at each call.
    // Release @0x155360 IS CDDrawWorkerRegistry::RemoveKeysEqual_155360; cast at each call.
    // Frame-name reverse-lookup (given a frame handle, write its name into tmp;
    // *outZero gets a found-index). Was the CStrReader / CMiNameReg placeholder views.
    void ReadField(i32 handle, char* tmp, i32* outZero); // 0x155630 __thiscall
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void Install(void* set, const char* szName, const char* szKey); // slot 18 (+0x48)
    // slot 19 (+0x4c): register a resolved symbol tree under a prefix (returns -1 on
    // failure) - the RESOURCE-facet op the game-state activators reach (CBootyState/
    // CMenuState/CPlay slot-8 loaders). Same +0x10 registrar object as Install.
    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);

    char m_pad04[0x10 - 0x4]; // vptr occupies +0x00..+0x03
    CSpriteHashTable m_10map; // +0x10  the name->sprite hash table
};

// The sound registry at CResMgr+0x28 (plain non-virtual helpers) + its +0x10 map + the
// pooled resource at +0x2c. The render/resource state TUs reach it as CState::m_c->m_28
// (the former View.h CViewSoundRegistry view folds here).
SIZE_UNKNOWN(CSoundRegistry);
struct CSoundRegistry {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
    char m_pad14[0x2c - 0x14];
    CViewPooledRes* m_2c; // +0x2c  pooled resource (Free() if set)
};

// The animation registry at CResMgr+0x2c (plain non-virtual helpers) + its +0x10 map.
// The render/resource state TUs reach it as CState::m_c->m_animRegistry (former
// View.h CViewAnimRegistry).
SIZE_UNKNOWN(CAnimRegistry);
struct CAnimRegistry {
    i32 Has(const char* szName);                          // 0x152c50 __thiscall, ret found
    void Register(const char* szName, const char* szKey); // 0x152720 __thiscall
    void Release(const char* szName, const char* szKey);  // 0x152720 (credits reg, loader alias)
    void Install(void* set, const char* szName, const char* szKey); // 0x152ad0 __thiscall
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
};

// The game's resource/level manager. Different callers pull the manager they need
// out of its slots: the draw target (+0x04), the world key table (+0x08), the
// image registry (+0x10), a second sprite-set registry used by CreateSprite (+0x14),
// the level/view object (+0x24), the sound registry (+0x28) and the anim registry
// (+0x2c).
SIZE_UNKNOWN(CResMgr);
struct CResMgr {
    char m_pad00[0x4];
    CDrawTarget* m_drawTarget; // +0x04  active draw surface
    CKeyTable* m_8;            // +0x08  world/key lookup table
    char m_pad0c[0x10 - 0xc];
    CImageRegistry* m_10; // +0x10  image/tile registry (sprite lookups + install)
    CImageRegistry* m_14; // +0x14  sprite-set registry (CreateSprite lookup)
    char m_pad18[0x24 - 0x18];
    CMenuViewObj* m_view; // +0x24  level/view object
    CSoundRegistry* m_28; // +0x28  sound registry
    CAnimRegistry* m_2c;  // +0x2c  animation registry
};

// --- vtable catalog ---

#endif // GRUNTZ_RESMGR_H
