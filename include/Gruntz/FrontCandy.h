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
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    TILE_LOGIC_TAIL
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0xfa60 (slot-1 two-chain body; ??_7CFrontCandy slot 1)
    CFrontCandy(CGameObject* obj); // 0x0abfa0
    virtual ~CFrontCandy() OVERRIDE;
};
VTBL(CFrontCandy, 0x1e84ec);

#endif // GRUNTZ_CFRONTCANDY_H
