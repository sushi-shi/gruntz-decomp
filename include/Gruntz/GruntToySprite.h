// GruntToySprite.h - the "grunt has a toy" indicator sprite, a CUserLogic-
// derived game object (vftables 0x5e705c / 0x5e70b4). The 0x44 dtor folds the
// bare CUserLogic teardown (leaf-dtor archetype); SetCell stashes the grunt cell
// and clears a flag bit; Update tracks the grunt's screen position + layer.
//
// Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types
#include <Gruntz/SerialArchive.h>        // shared CSerialArchive (Read +0x2c / Write +0x30)

SIZE_UNKNOWN(CGruntToySprite);
VTBL(CGruntToySprite, 0x001e7b4c); // vtable_names -> code (RTTI game class)
class CGruntToySprite : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012260, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYSPRITE;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    CGruntToySprite(CGameObject* obj);      // 0x07f350 (ctor body in GruntToySprite.cpp)
    virtual ~CGruntToySprite() OVERRIDE;    // 0x0122b0 (folds the CUserLogic teardown)

    static void InitActReg();   // 0x07f540 (construct g_toyActReg over [2000,2010])
    void RunAct(i32 id);        // 0x07f5c0 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07f720 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y); // 0x07f920
    i32 Update();              // 0x07f960
    // 0x07fa20: the serialize override - round-trip m_cellX/m_cellY (8 B) + m_lastLayer
    // (4 B) per mode (4=write @+0x30, 7=read @+0x2c), then chain CUserLogic::SerializeMove
    // and the +0x34 serialized-object-reference; return whether the ref chain succeeded.

    CAniElement*
        m_geoId; // +0x40  geometry-id cache slot (indicator-sprite family; unset by this leaf's ApplyLookupSprite ctor)
    char m_pad44[0x54 - 0x44];
    i32 m_cellX;     // +0x54  grunt cell x
    i32 m_cellY;     // +0x58  grunt cell y
    i32 m_lastLayer; // +0x5c  last-seen layer index (Update tracks layer change)
};

// The class registry entry: its first dword receives the Update handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CGruntToySprite::*ToyActHandler)();
SIZE_UNKNOWN(CToyActEntry);
struct CToyActEntry {
    ToyActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
