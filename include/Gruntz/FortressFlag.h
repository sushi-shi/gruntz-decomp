#ifndef GRUNTZ_CFORTRESSFLAG_H
#define GRUNTZ_CFORTRESSFLAG_H

#include <Ints.h>

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
    virtual void FireActivation(i32 id)  ;
    i32 AdvanceAnim();

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_FORTRESSFLAG;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
};

#endif
