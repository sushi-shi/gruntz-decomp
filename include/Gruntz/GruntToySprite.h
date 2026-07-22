#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types
#include <Gruntz/SerialArchive.h>        // shared CSerialArchive (Read +0x2c / Write +0x30)

class CGruntToySprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012260, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYSPRITE;
    } // slot 2
    CGruntToySprite(CGameObject* obj);   // 0x07f350 (ctor body in GruntToySprite.cpp)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    static void InitActReg(); // 0x07f540 (construct g_toyActReg over [2000,2010])
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x07f5c0 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07f720 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y); // 0x07f920
    i32 Update();              // 0x07f960
    // 0x07fa20: the serialize override - round-trip m_cellX/m_cellY (8 B) + m_lastLayer
    // (4 B) per mode (4=write @+0x30, 7=read @+0x2c), then chain CUserLogic::SerializeMove
    // and the +0x34 serialized-object-reference; return whether the ref chain succeeded.

    CAniElement*
        m_geoId; // +0x40  geometry-id cache slot (indicator-sprite family; unset by this leaf's ApplyLookupSprite ctor)
    i32 m_cellX;     // +0x54  grunt cell x
    i32 m_cellY;     // +0x58  grunt cell y
    i32 m_lastLayer; // +0x5c  last-seen layer index (Update tracks layer change)
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*ToyActHandler)();
struct CToyActEntry {
    ToyActHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
