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
#include <Gruntz/Sprite.h> // CMapStringToOb (each registry's +0x10 map)

struct CMenuViewObj;       // the level/view object at CResMgr+0x24 (ActionOptionsMenuBar-local)
struct IDirectDrawSurface; // CDrawTarget::SurfaceA::Surface2c::m_8 (the real DDraw surface: the
                           // credits/splash "poll" is IsLost (slot 24); GetDC/ReleaseDC at 17/26)
class CDDSurface;          // CDrawTarget::SurfaceB::m_2c (the real DDraw view surface; DDSurface.h)

// The pooled resource the leaf states free before releasing their namespaces
// (m_28->m_2c). Reloc-masked __thiscall. (Was View.h's CViewPooledRes.)
SIZE_UNKNOWN(CViewPooledRes);
struct CViewPooledRes {
    // Free @? IS SoundStream::Stop; cast at each call.
    // TickAnim @? IS SoundDevice::PurgeVoiceList; cast at the call.
};

// The three surface pages at CDrawTarget +0x10/+0x14/+0x18 are the REAL
// CDDrawSurfacePair (<DDrawMgr/DDrawSurfacePair.h>). The former nested
// SurfaceA/SurfaceB views (2026-07-14, dissolved) agreed with it field-for-field:
// "+0x10/+0x14 pixel extent" == m_width/m_height, the "+0x1c blit-source RECT
// origin" == m_srcRect[0], the +0x2c CDDSurface == m_surface (Fill @0x13e760 /
// Restore @0x13e7d0 / Flip @0x13e850), and SurfaceB::Blit IS
// CDDrawSurfacePair::BltSelf @0x3a1d0 (xref-proven: CCreditsState::Render's blit
// call -> thunk 0x1564 -> 0x3a1d0). GlyphStringDraw even cast SurfaceA* to
// SurfaceB* - one class all along. Pointer-only here (fwd decl) so this header
// stays afx-neutral; dereferencing TUs pull the real header.
class CDDrawSurfacePair;

// The active draw surface / render-flip pump at CResMgr+0x04. The loaders read its
// +0x14 as an opaque draw-context handle; the render/credits path drives it as the flip
// pump: Flush (0x158ee0 == CDDrawWorkerMgr::Method_158ee0 - the state activators call the
// SAME fn as the resource worker-apply on activate), the frame-surface page (+0x10), the
// draw page (+0x14) and the present target (+0x18). (This is the real +0x04 class; the
// former View.h StateMgrBZ view folds here. Its three pages mirror CDDrawSubMgrPages'
// m_frontPair/m_backPair/m_overlayPair EXACTLY - the CDrawTarget==CDDrawSubMgrPages
// identity is documented at the CSpriteFactoryHolder union in GameRegistry.h.)
SIZE_UNKNOWN(CDrawTarget);
struct CDrawTarget {
    char m_pad00[0x10];
    CDDrawSurfacePair* m_10; // +0x10  frame-surface page (== Pages::m_frontPair; CChatBox's
                             //        0x182ab0 seeder reads its m_width/m_height extent)
    CDDrawSurfacePair* m_14; // +0x14  draw page (== Pages::m_backPair)
    // +0x18  the present/back-buffer page (== Pages::m_overlayPair); the status-screen
    // overlay (CPlay/CMulti::FrameSlot28) clears it via m_surface->Fill(0).
    CDDrawSurfacePair* m_18;
};

// The world/key lookup table at CResMgr+0x08 the timer-expiry path probes
// (FindByKey / engine "Lookup" @0x1b8760, reached at +0x48). Modeled NO-body.
//
// The +0x48 probe target is now a REAL typed member: it is the MFC CMapPtrToPtr whose
// Lookup IS 0x1b8760 (the same class+rva CDDrawChildGroup's m_map2c/m_map48 bind,
// FID-confirmed). WwdGameObject's kill-cue path calls it directly as
// m_mgr->m_8->m_map48.Lookup(node, found) - it used to reach the identical hop through
// a `WwdMgrSub08` + `CMapStringToObLite` view pair, which this member dissolves.
SIZE_UNKNOWN(CKeyTable);
struct CKeyTable {
    i32 FindByKey(i32 key, i32* outFound);

    char m_pad00[0x48];   // +0x00..0x47
    CMapPtrToPtr m_map48; // +0x48  key -> object (Lookup 0x1b8760)
};

// The image/tile registry at CResMgr+0x10: a virtual Install at vtable slot 18
// (+0x48) plus non-virtual Has/Register helpers, and the name->sprite hash table
// embedded at its own +0x10. All methods external/no-body so the calls reloc-mask.
//
// This is the ONE image/name registry class - the render/resource state TUs reach it as
// CState::m_c->m_10 (the former View.h CSpriteFactoryHolder::CImageRegistry view folds here); the
// int-keyed frame-grid Lookup at CPlay::BeginGridWalk casts to the name-keyed m_10map.Lookup.
// IT IS THE CANONICAL CDDrawWorkerRegistry (<DDrawMgr/DDrawWorkerRegistry.h>, vtable
// ??_7CDDrawWorkerRegistry @0x1efd28, 23 slots ALL named from the retail slot map). The
// 18-filler view that stood here is dissolved: same object (vptr@0, CMapStringToOb@+0x10),
// same slot 18 (+0x48) install and slot 19 (+0x4c) LoadNamespace, and its own comments
// already conceded the point ("Has @0x155550 IS CDDrawWorkerRegistry::HasKeyEqual_155550").
// The recorded "FOREIGN object / honest manual-vtbl form" WALL was about converting the
// class to a PMF vtable struct (which flipped CSBI_MenuItem::DecCounter 100->92) - that is
// NOT what this fold does: the class stays real-polymorphic, it just becomes the ONE class
// instead of a filler twin, so DecCounter's schedule is untouched.
#include <DDrawMgr/DDrawWorkerRegistry.h>
typedef CDDrawWorkerRegistry CImageRegistry;

// The sound registry at CResMgr+0x28 (plain non-virtual helpers) + its +0x10 map + the
// pooled resource at +0x2c. The render/resource state TUs reach it as CState::m_c->m_28
// (the former View.h CViewSoundRegistry view folds here).
SIZE_UNKNOWN(CSoundRegistry);
struct CSoundRegistry {
    char m_pad00[0x10];
    CMapStringToOb m_10map; // +0x10
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
    CMapStringToOb m_10map; // +0x10
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
