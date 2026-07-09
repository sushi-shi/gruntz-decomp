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
#include <Gruntz/SerialObjRef.h> // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/UserLogic.h>    // CUserLogic base (CCursorSnapSprite : CUserLogic)

// ---------------------------------------------------------------------------
// CCursorSnapSprite : CUserLogic - the cursor-snap sprite leaf. Adds no data
// members the matched methods touch; the +0x34 serializable sub-object overlays
// the CUserLogic layout. The CUserLogic base gives the +0x18 destructible link,
// so the dtor folds the shared teardown.
// ---------------------------------------------------------------------------
class CCursorSnapSprite : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    // Serialize (0x11880): chain the shared CUserLogic serialize helper on `this`,
    // then (only on success) the +0x34 sub-object's chain; both run the same
    // (ar, tag, c, d) tuple. Returns the second chain's success as a bool.
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
    CCursorSnapSprite(CGameObject* obj); // 0x3a340
    // FireActivation (0x3a5b0): slot-4 (UserLogicVfunc2) override - resolve `id`
    // in the class dispatch table g_logicActReg_62bfa0; if the resolved entry holds
    // a registered handler, re-resolve and dispatch it __thiscall on `this`. Same
    // archetype as CTeleporter::FireActivation (double ResolveEntry + PMF dispatch).
    void FireActivation(i32 id);
    virtual ~CCursorSnapSprite() OVERRIDE; // 0x11920 (folds the CUserLogic teardown)

    i32 m_geoId; // +0x40  cached bound-object geometry id (ctor: m_38->m_geoId)
    char m_pad44[0x54 - 0x44]; // +0x44  (unmodeled tail; size proven 0x54 from the
                               //         anim-worker `new CCursorSnapSprite`)
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
typedef i32 (CCursorSnapSprite::*SnapActHandler)();
struct CSnapActEntry {
    SnapActHandler m_fn;
};
SIZE_UNKNOWN(CSnapActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CCURSORSNAPSPRITE_H
