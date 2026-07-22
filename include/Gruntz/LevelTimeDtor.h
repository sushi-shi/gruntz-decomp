#ifndef GRUNTZ_CLEVELTIMEDTOR_H
#define GRUNTZ_CLEVELTIMEDTOR_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CLevelTime : CUserLogic)

class CLevelTime : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011990, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LEVELTIME;
    } // slot 2
public:
    CLevelTime(CGameObject* obj);   // 0x9b8b0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // +0x40..+0x53: the level-timer state tail. NOT initialized by the ctor
    // (0x9b8b0 writes nothing past +0x3c - lazily-used fields); their existence +
    // the 0x54 size are pinned by the StateDispatch new-site (0x9b770: push 0x54
    // before operator new, then the bare `call ??0CLevelTime` thunk 0x404d).
};
SIZE(0x54); // `new CLevelTime` @0x9b77f pushes 0x54
VTBL(CLevelTime, 0x1e801c);

#endif // GRUNTZ_CLEVELTIMEDTOR_H
