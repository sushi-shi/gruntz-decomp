// SpriteFactory.h - the global HUD/game sprite factory (C:\Proj\Gruntz).
//
// ONE class, ONE definition (previously duplicated with three divergent inline
// views across SpriteResource.cpp / DemoSetup.cpp / IconLoaders.cpp).
// CreateSprite (@0x1597b0) looks a sprite TEMPLATE up by class-NAME (the 5th arg)
// in the factory's sprite-set table (m_c->m_14->map) and forwards the build args +
// the resolved template to CreateSpriteImpl (@0x159600), which `new`s the
// 0x1dc-byte game-sprite INSTANCE and constructs it. The created instance is the
// shared engine game object (CGameObject, <Gruntz/UserLogic.h>: it owns the
// ApplyName @0x150540 / ApplyLookupGeometry @0x1505b0 / ApplyLookupSprite
// @0x1504d0 setters every creating TU drives on the result, plus the +0x7c
// AnimWorkerObj control block whose Init @+0x10 the creators run post-create), so
// CreateSprite returns CGameObject*. The former per-TU created-instance names
// (CIconSprite / CHudSprite / CProjSprite / ...) were views of this one class.
// The lookup RESULT (the frame-data template) is the separate CSprite value type.
//
// The factory also owns the live created/placed display-object list at +0x14
// (walked by LoadToyBoxIcon to de-dup icons by class + tile, and by TriggerMgr's
// DestroyAllAnims to clear grunt channel markers - it holds every live created
// object, not just icons). Only offsets + code bytes are load-bearing; the field
// names are placeholders for the engine identities recovered from the call sites.
#ifndef GRUNTZ_SPRITEFACTORY_H
#define GRUNTZ_SPRITEFACTORY_H

#include <Ints.h>
#include <rva.h>

struct CResMgr;       // resource mgr at +0xc (m_14 = the factory's sprite-set registry)
struct CSprite;       // frame-data template value (the lookup RESULT)
struct CGameObject;   // the created 0x1dc-byte game-sprite INSTANCE (<Gruntz/UserLogic.h>)
class CWwdGameObject; // AttachSprite's already-allocated target (<Gruntz/WwdGameObject.h>;
                      // the ex-CSprite2 view is dissolved onto it)

// An entry in the factory's live-object list (m_liveObjects): next @+0, object @+8.
struct CSpriteListNode {
    CSpriteListNode* next; // +0x00
    char m_pad4[0x8 - 0x4];
    CGameObject* m_sprite; // +0x08
};
SIZE_UNKNOWN(CSpriteListNode);

// (The former GruntObjEntry view of the serialize key->object map entry is
// DISSOLVED onto the real wide-object family base CWwdGameObjectE
// (<Wwd/WwdGameObjectFamily.h>): its "Kind()"/GetTypeId was the family's slot-8
// GetClassId (the ==5 probe) and its +0x7c inner the canonical AnimWorkerObj.
// The map-walking consumer TUs include the family header themselves.)
class CWwdGameObjectE;
SIZE_UNKNOWN(GruntObjMap);
// MFC CMapPtrToPtr (the serialize-time key->object map). Lookup @0x1b8760 is
// declared-only (external, no body) so the __thiscall `lea ecx,[&map]; call` falls
// out reloc-masked - the MFC-free consumers reach it without pulling <afx.h>.
// The stored values ARE the created game sprites, so the out-param is typed
// CGameObject*& (kills the per-site (CGameObject*) retrieval casts; the mangled
// name shift is free - the symbol was reloc-masked/undefined either way).
struct GruntObjMap {
    i32 Lookup(void* key, CGameObject*& out); // 0x1b8760 (CMapPtrToPtr::Lookup)
};
class CSpriteFactory {
public:
    // The factory is polymorphic (vptr @+0x00) and IS the object manager - the
    // CWwdObjMgr == CDDrawChildGroup identity (vtable 0x1efdc0, 17 slots): the
    // per-frame render path's "slot 9 FrameBegin(flag) / slot 10 FramePresent
    // (drawSurface)" (CMultiBootyState::Render) are exactly the canonical's
    // TickKillCues_159a70(advance) / WalkDispatch2C(renderCtx). Slot names are
    // the canonical's (<DDrawMgr/DDrawChildGroup.h>); declared-only, reloc-masked.
    virtual void GetRuntimeClass();         // [0]  CObject slot (0x1bef01)
    virtual void ScalarDtor();              // [1]  0x157610
    virtual void Serialize();               // [2]  CObject slot (0x0028ec)
    virtual void AssertValid();             // [3]  CObject slot (0x00106e)
    virtual void Dump();                    // [4]  CObject slot (0x004034)
    virtual i32 IsLoaded();                 // [5]  0x1575e0
    virtual i32 IsReady();                  // [6]  0x1576c0
    virtual void ForwardTo3C();             // [7]  0x1591e0
    virtual i32 GetStateId();               // [8]  0x157600 (STATE_CHILDGROUP)
    virtual void TickKillCues(i32 advance); // slot 9  (+0x24) 0x159a70 per-frame kill-cue
                                            //         tick (was this view's "FrameBegin")
    virtual void WalkDispatch2C(void* ctx); // slot 10 (+0x28) 0x159c90 per-object render
                                            //         broadcast (was "FramePresent")

    // Public entry: look the template up by class-NAME, forward to the impl. __thiscall,
    // ret 0x18. Returns the created instance (or 0 if the template is missing).
    CGameObject* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // (The ex-`CreateSpriteImpl` decl was a PHANTOM second name for
    // CWwdObjMgr::CreateObject_159600 - CreateSprite calls the real method on the
    // shared `this`; the factory IS the object manager.)
    // Initialise an already-allocated sprite against a named template. __thiscall, ret 0x18.
    i32 AttachSprite(CWwdGameObject* obj, i32 a1, i32 a2, i32 a3, const char* name, i32 flags);
    // 0x159e40 is CWwdObjMgr::InsertSorted_159e40 (the factory IS the object manager);
    // AttachSprite calls it through a CWwdObjMgr* view - no fake local placeholder.

    char m_pad04[0xc - 0x4]; // vptr occupies +0x00..+0x03
    // @identity-TODO (item-2 merge prerequisite): this +0x0c owner is read as CResMgr*
    // here (m_c->m_14 sprite-set registry) but as CDDrawSurfaceMgr* in the CWwdObjMgr view
    // (m_0c->m_workerCache) - the SAME field of the SAME object (CSpriteFactory == CWwdObjMgr
    // == CDDrawChildGroup, vtable 0x1efdc0). Folding the trio to one class requires first
    // resolving whether CResMgr == CDDrawSurfaceMgr (a nested identity) or one view mistypes
    // +0x0c; do that before the ~50-80 file rename so the merged field carries one true type.
    CResMgr* m_c; // +0x0c
    char m_pad10[0x14 - 0x10];
    CSpriteListNode* m_liveObjects; // +0x14  live created-object list head
    char m_pad18[0x48 - 0x18];
    GruntObjMap m_objMap; // +0x48  embedded key->object map (Lookup @0x1b8760)
    // +0x48: an embedded MFC CMapPtrToPtr (the serialize-time key->object map that
    // CTriggerMgr::Load resolves record keys through, Lookup @0x1b8760). Not typed here
    // because this Win32-included header must stay MFC-free (afx C1189); the MFC consumer
    // reaches it as CMapPtrToPtr at the documented +0x48 offset.
};
SIZE_UNKNOWN(CSpriteFactory);

// --- vtable catalog ---

#endif // GRUNTZ_SPRITEFACTORY_H
