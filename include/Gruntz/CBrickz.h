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
    CBrickz(CGameObject* obj); // 0x10e800 (1-arg ctor)
    ~CBrickz();                // (folds the CUserLogic teardown; vtable anchor)
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    LogicTypeId GetTypeTag();                  // 0x11300 (vtable slot 2: per-class logic-type id)
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x11320 (vtable slot 1: serialize chain)

    // CBrickz's own data begins at +0x40 (CUserLogic ends at +0x40); the 1-arg
    // ctor touches none of it. The leaf is 0x54 (0x40 base + 0x14 own).
    char m_own[0x54 - 0x40];
};

#endif // GRUNTZ_CBRICKZ_H
