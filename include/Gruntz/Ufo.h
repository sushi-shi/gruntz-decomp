// Ufo.h - the UFO path-hazard game-object (C:\Proj\Gruntz), a CPathHazard leaf
// (RTTI-proven; vtable_hierarchy --tree). The real base is the fully-modeled
// 21-slot CPathHazard (<Gruntz/PathHazard.h>). Adds no data members over the
// 0x130-byte base; cl emits its own ??_7CUFO + the implicit post-base-ctor vptr
// stamp. Only offsets / code bytes are load-bearing.
#ifndef GRUNTZ_CUFO_H
#define GRUNTZ_CUFO_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/PathHazard.h>  // CPathHazard base (+ CGruntArchive / CGameObject)

class CUFO : public CPathHazard {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    CUFO(CGameObject* obj);
    virtual ~CUFO() OVERRIDE;    // slot 0
    virtual i32 Tick() OVERRIDE; // slot 16
    // CUFO::SerializeMove (slot 1, 0xb4c40) is the real override (defined in
    // GameObjectCtors.cpp): it wraps this non-virtual field-transfer helper and, on
    // mode 8, re-seeds the ctor's draw-fill state.
    i32 Serialize(void* stream, i32 tag, i32 c, i32 d);      // 0x0b4d30
    i32 SerializeChain(void* stream, i32 tag, i32 c, i32 d); // 0x16e7f0 (base chain; call-only)
};
VTBL(CUFO, 0x001e72b4); // vtable_names -> code (RTTI game class)
SIZE(CUFO, 0x130);

#endif // GRUNTZ_CUFO_H
