#ifndef GRUNTZ_GRUNTZ_CINGAMETEXT_H
#define GRUNTZ_GRUNTZ_CINGAMETEXT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CInGameText : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00011d70, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMETEXT;
    }

public:
    CInGameText() {}
    CInGameText(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    i32 UpdateHelpBook();

    i32 m_lastReaderPlayerIndex;
    i32 m_lastReaderUnitIndex;
};

#endif // GRUNTZ_GRUNTZ_CINGAMETEXT_H
