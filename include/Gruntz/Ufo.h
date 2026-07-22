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
    // NO user-declared dtor: retail's ??1 @0x13400 stamps ONLY the CUserBase vtable
    // (all intermediate CUFO/CPathHazard stamps dead-store-eliminated) - the IMPLICIT
    // compiler-generated dtor reproduces that elision (the CDoNothingNormal precedent);
    // an explicit body emits the intermediate stamps (byte-proven 4.7% crater).
    // Emitter + @rva-symbol pin: src/Gruntz/GruntVoice.cpp (the 0x13400 band's TU).
    virtual i32 Tick() OVERRIDE; // slot 16
    // CUFO::SerializeMove (slot 1, 0xb4c40) is the real override (defined in
    // GameObjectCtors.cpp): it wraps this non-virtual field-transfer helper and, on
    // mode 8, re-seeds the ctor's draw-fill state.
    // 0x0b4cb0: serialize-then-decorate variant (exact name unrecovered); re-homed
    // from AppHelpers.cpp (was CHandlerB4::Handle - proven a CUFO method by xref).
    i32 Method_b4cb0(void* stream, i32 tag, i32 c, i32 d);
};
SIZE(0x130);
VTBL(CUFO, 0x001e72b4); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_CUFO_H
