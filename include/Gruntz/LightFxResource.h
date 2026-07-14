// LightFxResource.h - the object-owner-context resource sub-system CLightFx::Activate
// walks (C:\Proj\Gruntz).
//
// @identity-TODO. The holder + the two stores are partial views of a REAL but
// still-unconsolidated engine resource family: the holder IS the object's owner
// context (CGameObjWorld, reached identically by AnimWorkerObj::m_0c and
// CGameObject::m_0c - both point at the one context; its canonical in
// <Gruntz/UserLogic.h> models +0x08/+0x14/+0x24 but not the +0x10/+0x2c store
// pointers these probes read, and adding them there would drag <Mfc.h> into that
// 66-TU MFC-free header). The store shape (a pad + an MFC string map at +0x10) is a
// SHARED name->resource holder still modeled per-TU across ~8 headers (ResMgr /
// DDrawWorkerCtx / CProjSpriteMgr / MenuItem / StatusBarMgr / ...); until that
// family is consolidated into one canonical, these stay as minimal honest views
// that wrap the REAL MFC maps. Kept in this header (not inline in the .cpp) so they
// are not a per-TU divergent definition. The two store maps are DIFFERENT MFC types,
// proven by disasm of CLightFx::Activate @0x9d520: the spec lookup is `call 0x1b8008`
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
