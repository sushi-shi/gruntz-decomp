// GruntSelectedSprite.h - the "grunt is selected" indicator sprite, a
// CUserLogic-derived game object (vftables 0x5e705c / 0x5e70b4). The 0x44 dtor
// folds the bare CUserLogic teardown (the leaf-dtor archetype); SetCell stores
// the (x,y) grunt cell; Update tracks the selected grunt's screen position.
//
// Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTSELECTEDSPRITE_H
#define GRUNTZ_CGRUNTSELECTEDSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types

SIZE_UNKNOWN(CGruntSelectedSprite);
class CGruntSelectedSprite : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1 (0x07ea70)
    RVA(0x00011e30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSELECTEDSPRITE;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;   // slot 4
    CGruntSelectedSprite(CGameObject* obj);   // 0x07e3e0 (ctor body in GruntSelectedSprite.cpp)
    virtual ~CGruntSelectedSprite() OVERRIDE; // 0x011e80 (folds the CUserLogic teardown)

    static void InitActReg();   // 0x07e5e0 (construct g_selectedActReg over [2000,2010])
    void RunAct(i32 id);        // 0x07e660 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07e7c0 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y); // 0x07e9c0
    i32 Update();              // 0x07e9f0

    CAniElement* m_geoId; // +0x40  cached bound-object geometry id (ctor: m_38->m_1a0.m_14)
    char m_pad44[0x54 - 0x44];
    i32 m_cellX; // +0x54  grunt cell x
    i32 m_cellY; // +0x58  grunt cell y
};
VTBL(CGruntSelectedSprite, 0x001e7bfc); // vtable_names -> code (RTTI game class)

// The class registry entry: its first dword receives the Update handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CGruntSelectedSprite::*SelectedActHandler)();
SIZE_UNKNOWN(CSelectedActEntry);
struct CSelectedActEntry {
    SelectedActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTSELECTEDSPRITE_H
