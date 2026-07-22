#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CDroppedObjectShadow : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012620, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECTSHADOW;
    } // slot 2
public:
    CDroppedObjectShadow(CGameObject* obj);   // 0xc7490 (1-arg leaf ctor)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    // The slot-1 serialize impl (plain method: ?Serialize name + RVA pin, vtable
    // slot reloc-masked, like CDroppedObject::Serialize).
    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, i32 d); // 0xc7b40
    // The activation-registry facet (ex ActRegSiblings.cpp's "CSiblingActorB" -
    // identity recovered: its registry construct 0xc76d0 sits right after this
    // class's ctor, and its per-frame Advance spawns the "DroppedObject" sprite
    // on the drop frame - the shadow IS the dropper's drop herald):
    static void InitActReg();                     // 0xc76d0 (build g_shadowActReg over [2000,2010])
    virtual void FireActivation(i32 id) OVERRIDE; // 0xc7750 (look up + fire the registered handler)
    static void RegisterActs();                   // 0xc78b0 (bind Advance to the "A" key)
    i32 Advance();             // 0xc7ab0 (per-frame: advance anim; drop frame -> spawn)
};
SIZE(0x54);

struct CShadowActEntry {
    i32 (CUserLogic::*m_fn)();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
