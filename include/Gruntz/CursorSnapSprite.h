// CursorSnapSprite.h - the cursor-snap sprite game object (C:\Proj\Gruntz).
//
// CCursorSnapSprite : CUserLogic (RTTI: .?AVCCursorSnapSprite@@ at 0x609b70). A
// tile-logic leaf in the same game-object hierarchy as CFortressFlag, proven by
// its dtor (0x11920) stamping the CUserLogic vftable 0x5e705c then the CUserBase
// vftable 0x5e70b4, tearing down the +0x18 link via the embedded ~EngStr at
// 0x16d2a0 (the /GX leaf-dtor archetype). The leaf adds no destructible members
// beyond CUserLogic, so the dtor folds the bare teardown.
//
// Serialize (0x11880) is the bare two-chain Serialize override: the shared
// CUserLogic serialize helper on `this`, then the +0x34 sub-object's chain - the
// SAME archetype as CFortressFlag::Serialize, minus the tag-8 fixup.
//
// Field names are placeholders; only the OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CCURSORSNAPSPRITE_H
#define GRUNTZ_CCURSORSNAPSPRITE_H

#include <rva.h>

#include <Gruntz/ActReg.h>       // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/UserLogic.h>    // CUserLogic base (CCursorSnapSprite : CUserLogic)

// ---------------------------------------------------------------------------
// CCursorSnapSprite : CUserLogic - the cursor-snap sprite leaf. Adds no data
// members the matched methods touch; the +0x34 serializable sub-object overlays
// the CUserLogic layout. The CUserLogic base gives the +0x18 destructible link,
// so the dtor folds the shared teardown.
// ---------------------------------------------------------------------------
class CCursorSnapSprite : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
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
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
VTBL(CCursorSnapSprite, 0x1e8074);
SIZE(CCursorSnapSprite, 0x54);

// The class's activation-dispatch table (CLogicActTable @0x62bfa0); filled by
// RegisterXLogic_62bfa0 (LogicActReg.cpp), read by FireActivation. Owner (DATA
// binding) is LogicActReg.cpp; declared extern here so the loads reloc-mask.
extern CLogicActTable g_logicActReg_62bfa0;

// A dispatch-table entry: its first dword is the class activation handler, stored
// by the registrar as a free-fn ptr but dispatched __thiscall on `this` - a 4-byte
// single-inheritance PMF gives the plain `mov ecx,this; call [entry]` code.
typedef i32 (CUserLogic::*SnapActHandler)();
struct CSnapActEntry {
    SnapActHandler m_fn;
};
SIZE_UNKNOWN(CSnapActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CCURSORSNAPSPRITE_H
