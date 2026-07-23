#ifndef GRUNTZ_CGRUNTSELECTEDSPRITE_H
#define GRUNTZ_CGRUNTSELECTEDSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types

class CGruntSelectedSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1 (0x07ea70)
    RVA(0x00011e30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSELECTEDSPRITE;
    } // slot 2
    CGruntSelectedSprite(CGameObject* obj); // 0x07e3e0 (ctor body in GruntSelectedSprite.cpp)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    static void InitActReg(); // 0x07e5e0 (construct g_selectedActReg over [2000,2010])
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x07e660 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07e7c0 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y); // 0x07e9c0
    i32 Update();              // 0x07e9f0
    i32 m_cellX;               // +0x54  grunt cell x
    i32 m_cellY;               // +0x58  grunt cell y
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*SelectedActHandler)();
struct CSelectedActEntry {
    SelectedActHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/GruntIndicatorSprite.h>  // CIndicatorActReg (extern below)
extern CIndicatorActReg g_selectedActReg; // 0x00244da8

#endif // GRUNTZ_CGRUNTSELECTEDSPRITE_H
