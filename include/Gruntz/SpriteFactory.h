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
// CGameObjAux control block whose Init @+0x10 the creators run post-create), so
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

struct CResMgr;     // resource mgr at +0xc (m_14 = the factory's sprite-set registry)
struct CSprite;     // frame-data template value (the lookup RESULT)
struct CGameObject; // the created 0x1dc-byte game-sprite INSTANCE (<Gruntz/UserLogic.h>)
// AttachSprite's already-allocated target; __single_inheritance keeps the init PMF a
// 4-byte code pointer (else MSVC's general PMF emits a this-adjust thunk).
struct __single_inheritance CSprite2;

// An entry in the factory's live-object list (m_liveObjects): next @+0, object @+8.
struct CSpriteListNode {
    CSpriteListNode* next; // +0x00
    char m_pad4[0x8 - 0x4];
    CGameObject* m_sprite; // +0x08
};
SIZE_UNKNOWN(CSpriteListNode);

struct CSpriteInner; // GruntObjEntry's +0x7c inner object (grunt world)
// The serialize key->object map entry (grunt world): its +0x20 Kind() virtual + the
// +0x7c inner object. Reached through the factory's embedded +0x48 map.
SIZE_UNKNOWN(GruntObjEntry);
class GruntObjEntry {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual i32 Kind(); // vtable slot +0x20
    char m_pad04[0x7c - 0x04];
    CSpriteInner* m_7c;         // +0x7c  inner object
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill7(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill8(); // vtable-slot filler (real slot; declared-only)
};
SIZE_UNKNOWN(GruntObjMap);
struct GruntObjMap {}; // MFC CMapPtrToPtr (Lookup @0x1b8760); cast at the call
class CSpriteFactory {
public:
    // Public entry: look the template up by class-NAME, forward to the impl. __thiscall,
    // ret 0x18. Returns the created instance (or 0 if the template is missing).
    CGameObject* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // The real allocator/ctor @0x159600 (external/no-body so the call reloc-masks).
    CGameObject* CreateSpriteImpl(i32 kind, i32 geoB, i32 geoA, i32 hint, CSprite* tmpl, i32 flags);
    // Initialise an already-allocated sprite against a named template. __thiscall, ret 0x18.
    i32 AttachSprite(CSprite2* obj, i32 a1, i32 a2, i32 a3, const char* name, i32 flags);
    void AddChild(CSprite2* obj, i32 flag); // 0x159e40 (external/no-body)

    char m_pad00[0xc];
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
VTBL(GruntObjEntry, 0x001e798c);

#endif // GRUNTZ_SPRITEFACTORY_H
