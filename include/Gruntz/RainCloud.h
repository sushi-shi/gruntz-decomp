// RainCloud.h - the rain-cloud path-hazard game-object (C:\Proj\Gruntz), a
// CPathHazard leaf (RTTI-proven; vtable_hierarchy --tree). The real base is the
// fully-modeled 21-slot CPathHazard (<Gruntz/PathHazard.h>). Adds no data members
// over the 0x130-byte base; cl emits its own ??_7CRainCloud + the implicit post-
// base-ctor vptr stamp. Only offsets / code bytes are load-bearing.
#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/PathHazard.h>  // CPathHazard base (+ CGruntArchive / CGameObject)

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    CRainCloud(CGameObject* obj);
    // The slots CRainCloud overrides over CPathHazard's vtable (declared only;
    // reloc-masked). slots 1/2 (origin CUserBase) stay inherited-attributed.
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    virtual i32 Tick() OVERRIDE;            // slot 16
    virtual i32 HitTest(i32, i32) OVERRIDE; // slot 20
};
VTBL(CRainCloud, 0x001e7324); // vtable_names -> code (RTTI game class)
SIZE(CRainCloud, 0x130);

#endif // GRUNTZ_CRAINCLOUD_H
