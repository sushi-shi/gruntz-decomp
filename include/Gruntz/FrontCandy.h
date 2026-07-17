// FrontCandy.h - a front-candy eyecandy game-object (C:\Proj\Gruntz), a CUserLogic
// tile-logic leaf in the candy family (sibling of CBehindCandy/CEyeCandy). Extracted
// from the former UserLogic.cpp-local view so the leaf dtor (0xfb00) homes onto the
// real class. Only offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CFRONTCANDY_H
#define GRUNTZ_CFRONTCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFrontCandy : CUserLogic)

SIZE_UNKNOWN(CFrontCandy);
class CFrontCandy : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in UserLogic.cpp)
    RVA(0x0000fa40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDY;
    }
    TILE_LOGIC_TAIL
public:
    CFrontCandy(CGameObject* obj); // 0x0abfa0
    virtual ~CFrontCandy() OVERRIDE;
    char m_pad40[0x54 - 0x40]; // +0x40..0x53 (leaf is 0x54: its only new-site, the
                               // logic-worker pump @0xaa1e0, pushes 0x54)
};
VTBL(CFrontCandy, 0x1e84ec);

#endif // GRUNTZ_CFRONTCANDY_H
