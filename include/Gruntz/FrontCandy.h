#ifndef GRUNTZ_CFRONTCANDY_H
#define GRUNTZ_CFRONTCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFrontCandy : public CUserLogic, public CWapX {
public:
    RVA(0x0000fa60, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }

    RVA(0x0000fa40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDY;
    }

public:
    CFrontCandy() {}
    CFrontCandy(CGameObject* obj);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CFRONTCANDY_H
