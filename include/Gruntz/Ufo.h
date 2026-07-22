#ifndef GRUNTZ_CUFO_H
#define GRUNTZ_CUFO_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/PathHazard.h>  // CPathHazard base (+ CFileMemBase / CGameObject)

class CUFO : public CPathHazard {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1 @0xb4c40
    // GetTypeTag (0x133b0, ??_7CUFO slot 2 -> this body): CUFO's own 6-byte
    // copy - and it returns 0x426, NOT a per-class id distinct from every
    // sibling: the id sits between CRainCloud's 0x425 and CFortressFlag's
    // 0x427, so 0x426 IS the UFO id (the enum's old "LOGIC_VOICETRIGGER"
    // label at 0x426 came from this very misbinding).
    RVA(0x000133b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_UFO;
    }
    CUFO(CGameObject* obj);
    // NO user-declared dtor: retail's ??1 @0x13400 stamps ONLY the CUserBase vtable
    // (all intermediate CUFO/CPathHazard stamps dead-store-eliminated) - the IMPLICIT
    // compiler-generated dtor reproduces that elision (the CDoNothingNormal precedent);
    // an explicit body emits the intermediate stamps (byte-proven 4.7% crater).
    // Emitter + RVA_COMPGEN pin: src/Gruntz/GruntVoice.cpp (the 0x13400 band's TU).
    virtual i32 Tick() OVERRIDE; // slot 16
    // (0xb4cb0, the ex "Method_b4cb0", is CRainCloud::SerializeMove -
    //  ??_7CRainCloud[1] is its only vtable referent; see RainCloud.cpp.)
};
SIZE(0x130);

#endif // GRUNTZ_CUFO_H
