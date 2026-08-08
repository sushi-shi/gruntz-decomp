#ifndef GRUNTZ_CFRONTCANDY_H
#define GRUNTZ_CFRONTCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFrontCandy : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    RVA(0x0000fa40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDY;
    }

public:
    CFrontCandy() {}
    CFrontCandy(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_CFRONTCANDY_H
