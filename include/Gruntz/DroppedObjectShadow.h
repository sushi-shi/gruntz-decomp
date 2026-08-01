#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CDroppedObjectShadow : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00012620, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECTSHADOW;
    }

public:
    CDroppedObjectShadow() {}
    CDroppedObjectShadow(CGameObject* obj);

    i32 Serialize(CFileMemBase* ar, i32 tag, i32 c, i32 d);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Advance();
};
SIZE(0x54);

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
