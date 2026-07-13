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

struct AnimWorkerObj; // the canonical +0x7c worker/logic record (<DDrawMgr/AnimWorkerObj.h>)
// The serialize key->object map entry (grunt world), reached through the factory's
// embedded +0x48 map. It IS the wide game object (the CWwdGameObjectE family /
// CGameObject views): its "+0x20 Kind()" is the family's slot-8 GetTypeId (the
// same ==5 probe every FindBy* uses) and its +0x7c inner is the worker/aux block.
// Kept as the grunt-world walking view pending the wide fold (@identity-TODO);
// slots carry the canonical names (<Wwd/WwdGameObjectFamily.h>, table 0x5f0020).
SIZE_UNKNOWN(GruntObjEntry);
class GruntObjEntry {
public:
    virtual void GetRuntimeClass(); // [0]  CObject slot (0x1bef01)
    virtual void* Delete(i32 flag); // [1]  scalar-deleting dtor
    virtual void Serialize();       // [2]  CObject slot (0x0028ec)
    virtual void AssertValid();     // [3]  CObject slot (0x00106e)
    virtual void Dump();            // [4]  CObject slot (0x004034)
    virtual i32 IsLoaded();         // [5]  0x15b370 (worker-gate)
    virtual i32 IsReady();          // [6]  0x001c08 (CWapObj default)
    virtual void ReleaseSubs();     // [7]  0x15b5d0
    virtual i32 GetTypeId();        // [8]  +0x20  per-kind type tag (==5 probe;
                                    //      was this view's "Kind()")
    char m_pad04[0x7c - 0x04];
    AnimWorkerObj* m_7c; // +0x7c  the owned 0x17c worker/logic record
    // Slots 9-17 of the family table, declared-only (canonical names):
    virtual i32 SetPosition(i32 x, i32 y);                    // [9]  0x164790
    virtual i32 Setup28(i32 a1, i32 a2, i32 a3, i32 a4);      // [10] 0x150d60
    virtual void Render(void* ctx);                           // [11]
    virtual void BltDirty(void* a, void* b);                  // [12]
    virtual void BltDirtyEx(void* a, void* b, i32 c);         // [13]
    virtual void BltDirtyRegions(void* a, void* b, i32 c);    // [14]
    virtual i32 Play3C(i32 ar, i32 mode, i32 a3, void* self); // [15] 0x151150
    virtual i32 Vfunc40();                                    // [16] 0x1bef01
    virtual u8 GetDotColor();                                 // [17] (C kind)
};
SIZE_UNKNOWN(GruntObjMap);
// MFC CMapPtrToPtr (the serialize-time key->object map). Lookup @0x1b8760 is
// declared-only (external, no body) so the __thiscall `lea ecx,[&map]; call` falls
// out reloc-masked - the MFC-free consumers reach it without pulling <afx.h>.
struct GruntObjMap {
    i32 Lookup(void* key, void*& out); // 0x1b8760 (CMapPtrToPtr::Lookup)
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
    CResMgr* m_c;            // +0x0c
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
