#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

#include <Gruntz/UserLogic.h>

class CTileTriggerTransition : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
public:
    CTileTriggerTransition(CGameObject* obj); // 0x10faf0
    // NO user-declared dtor: retail 0x117f0 is COMPILER-GENERATED (implicit; pin in
    // TileLogicPump.cpp).

    // per-class logic-type id (0x405); body out-of-line at 0x011730 in the leaf pool.
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x10fd10 (vtable slot 4: per-coord PMF dispatch)
    static void RegisterActs(); // 0x10fe70  intern "A", bind Handler (static: no this)
    i32 ApplyAnimation(char* sprite, char* geom); // 0x110070
    i32 TransitionAct();                          // 0x110110  the per-frame handler bound here

    // Leaf fields: CUserLogic ends at +0x40, the leaf object is 0x54 (the size the
    // state pump's `operator new(0x54)` allocates). m_activeAnimDesc caches the
    // +0x1b4 animation descriptor.
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*TileActHandler)();
struct TileActEntry {
    TileActHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
