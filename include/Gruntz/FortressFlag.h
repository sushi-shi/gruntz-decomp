#ifndef GRUNTZ_CFORTRESSFLAG_H
#define GRUNTZ_CFORTRESSFLAG_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFortressFlag : public CUserLogic, public CWapX {
public:
public:
    CFortressFlag() {}
    CFortressFlag(CGameObject* obj);

    static void RegisterActs();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 AdvanceAnim();

    RVA(0x00010e40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FORTRESSFLAG;
    }
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
};

#endif // GRUNTZ_CFORTRESSFLAG_H
