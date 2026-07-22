#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/PathHazard.h>  // CPathHazard base (+ CFileMemBase / CGameObject)

class CRainCloud : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    CRainCloud(CGameObject* obj);
    // The slots CRainCloud overrides over CPathHazard's vtable (declared only;
    // reloc-masked). slots 1/2 (origin CUserBase) stay inherited-attributed.
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    virtual i32 Tick() OVERRIDE;            // slot 16
    virtual i32 HitTest(i32, i32) OVERRIDE; // slot 20
};
SIZE(0x130);

#endif // GRUNTZ_CRAINCLOUD_H
