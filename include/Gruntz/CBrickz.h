// CBrickz.h - the CBrickz game-object (C:\Proj\Gruntz).
//
// CBrickz : CUserLogic (RTTI; most-derived vftable 0x5e7c54). The "Brickz" puzzle
// tile-logic object - the standard tile-logic game-object family (the same shape
// as CTimeBomb / CStaticHazard / the CTileTrigger leaves), built by the 1-arg
// ctor (0x10e800) off a CGameObject. Its own pathfinding grid is a SEPARATE
// self-contained container sub-object (the placeholder "CBrickz" in
// <Gruntz/Brickz.h> - a DIFFERENT class, reached through a different `this`).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CBRICKZ_H
#define GRUNTZ_CBRICKZ_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CBrickz : CUserLogic)

class CBrickz : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CBrickz(CGameObject* obj);   // 0x10e800 (1-arg ctor)
    virtual ~CBrickz() OVERRIDE; // (folds the CUserLogic teardown; vtable anchor)
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    // 0x00011300 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00011300, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BRICKZ;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    // Declared-only (body 0x810f0, Brickz.cpp): load the puzzle's tile attributes.
    // Added so Play.cpp can call it on the real class instead of on the fabricated `Eng`
    // conflation (the call reloc-masks either way, but only this spelling binds to the
    // symbol retail actually calls).
    i32 LoadAttributes(i32 a, i32 b); // 0x0810f0

    // CBrickz's own data begins at +0x40 (CUserLogic ends at +0x40); the 1-arg
    // ctor touches none of it. The leaf is 0x54 (0x40 base + 0x14 own).
    char m_own[0x54 - 0x40];
};
VTBL(CBrickz, 0x001e7c54);
SIZE(CBrickz, 0x54); // 0x40 CUserLogic base + 0x14 own

#endif // GRUNTZ_CBRICKZ_H
