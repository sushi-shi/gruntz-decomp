// LightFxResource.h - the object-owner-context resource sub-system CLightFx::Activate
// walks (C:\Proj\Gruntz).
//
// Xref-recovered, partially. The holder IS the game object's owner context (obj->m_0c,
// reached identically by AnimWorkerObj::m_0c and CGameObject::m_0c). That context is
// itself a still-unconsolidated multi-view of ONE retail class: <Gruntz/UserLogic.h>
// models it as CGameObjWorld (+0x08 obj-chain / +0x14 worker-cache / +0x24 level),
// while src/Wwd/WwdGameObject.cpp reads the SAME obj->m_0c as CDDrawSurfaceMgr
// (m_0c->m_flags) - the surface/plane manager that doubles as the object world/asset
// context. Its +0x10 store is the name->CImageSet spec registry, +0x2c the
// name->CAniElement effect registry; the store shape (a pad + an MFC string map at
// +0x10) recurs per-TU across ~8 headers (ResMgr / DDrawWorkerCtx / CProjSpriteMgr /
// MenuItem / StatusBarMgr / ...). @identity-TODO: fully folding the stores onto the
// context needs that CGameObjWorld<->CDDrawSurfaceMgr conflation resolved into one
// canonical first (+ its store fields, which drag <Mfc.h> into the 66-TU UserLogic.h).
// Kept in this header (not inline in the .cpp) so they are not a per-TU divergent
// definition. The two store maps are DIFFERENT MFC types, proven by disasm of
// CLightFx::Activate @0x9d520: the spec lookup is `call 0x1b8008`
// (CMapStringToOb::Lookup), the effect lookups are `call 0x1b8438`
// (CMapStringToPtr::Lookup) - the labels used to be inverted here.
#ifndef GRUNTZ_LIGHTFXRESOURCE_H
#define GRUNTZ_LIGHTFXRESOURCE_H

#include <Mfc.h> // CMapStringToOb / CMapStringToPtr (the store maps)
#include <rva.h>

// The spec store: the MFC CMapStringToOb core sits at +0x10 (name -> CImageSet).
struct LfxSpecStore {
    char m_pad00[0x10];
    CMapStringToOb m_map; // +0x10  Lookup 0x1b8008
};
SIZE_UNKNOWN(LfxSpecStore);

// The effect store: the MFC CMapStringToPtr core sits at +0x10 (name -> CAniElement).
struct LfxEffectStore {
    char m_pad00[0x10];
    CMapStringToPtr m_map; // +0x10  Lookup 0x1b8438
};
SIZE_UNKNOWN(LfxEffectStore);

// The owner context's map holder (CGameObjWorld's +0x10/+0x2c resource region):
// +0x10 the spec store, +0x2c the effect store.
struct LfxMapHolder {
    char m_pad00[0x10];
    LfxSpecStore* m_spec; // +0x10  spec store (CMapStringToOb)
    char m_pad14[0x2c - 0x14];
    LfxEffectStore* m_effect; // +0x2c  effect store (CMapStringToPtr)
};
SIZE_UNKNOWN(LfxMapHolder);

#endif // GRUNTZ_LIGHTFXRESOURCE_H
