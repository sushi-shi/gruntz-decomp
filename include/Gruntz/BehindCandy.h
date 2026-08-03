#ifndef GRUNTZ_CBEHINDCANDY_H
#define GRUNTZ_CBEHINDCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBehindCandy : public CUserLogic, public CWapX {
public:
public:
    CBehindCandy() {}
    CBehindCandy(CGameObject* obj);

    RVA(0x0000fb70, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BEHINDCANDY;
    }
    RVA(0x0000fb90, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
};
SIZE(0x54);

#endif // GRUNTZ_CBEHINDCANDY_H
