#ifndef GRUNTZ_CCURSORSNAPSPRITE_H
#define GRUNTZ_CCURSORSNAPSPRITE_H

#include <rva.h>

#include <Gruntz/ActReg.h>        // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/UserLogic.h>     // CUserLogic base (CCursorSnapSprite : CUserLogic)

class CCursorSnapSprite : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011860, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_CURSORSNAPSPRITE;
    } // slot 2
public:
    // Serialize (0x11880): chain the shared CUserLogic serialize helper on `this`,
    // then (only on success) the +0x34 sub-object's chain; both run the same
    // (ar, tag, c, d) tuple. Returns the second chain's success as a bool.
    CCursorSnapSprite(CGameObject* obj); // 0x3a340
    // FireActivation (0x3a5b0): slot-4 (UserLogicVfunc2) override - resolve `id`
    // in the class dispatch table g_logicActReg_62bfa0; if the resolved entry holds
    // a registered handler, re-resolve and dispatch it __thiscall on `this`. Same
    // archetype as CTeleporter::FireActivation (double ResolveEntry + PMF dispatch).
    virtual void FireActivation(i32 id) OVERRIDE;
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE(0x54);

extern CLogicActTable g_logicActReg_62bfa0;

typedef i32 (CUserLogic::*SnapActHandler)();
struct CSnapActEntry {
    SnapActHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CCURSORSNAPSPRITE_H
