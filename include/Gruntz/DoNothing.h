#ifndef GRUNTZ_CDONOTHING_H
#define GRUNTZ_CDONOTHING_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CDoNothing : public CUserLogic, public CWapX {
public:
public:
    CDoNothing() {}
    CDoNothing(CGameObject* obj);

    RVA(0x0000f6b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHING;
    }
    RVA(0x0000f6d0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
};
SIZE(0x54);

#endif // GRUNTZ_CDONOTHING_H
