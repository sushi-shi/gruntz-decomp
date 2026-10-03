#ifndef GRUNTZ_GRUNTZ_CINGAMETEXT_H
#define GRUNTZ_GRUNTZ_CINGAMETEXT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CInGameText : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_INGAMETEXT;
    }

public:
    CInGameText() {}
    CInGameText(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    i32 Update();

    i32 m_cachedPlayerIndex;
    i32 m_cachedUnitIndex;
};

#endif
