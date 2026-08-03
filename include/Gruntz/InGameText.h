#ifndef GRUNTZ_GRUNTZ_CINGAMETEXT_H
#define GRUNTZ_GRUNTZ_CINGAMETEXT_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Wap32/ZVec.h>

class CFileMemBase;

class CInGameText : public CUserLogic, public CWapX {
public:
    RVA(0x00099a30, 0xaa)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId a, CGameObject* b)
        OVERRIDE {
        if (ar == 0) {
            return 0;
        }
        if (CUserLogic::SerializeMove(ar, tag, a, b) == 0) {
            return 0;
        }
        if (Chain(ar, tag, a, b) == 0) {
            return 0;
        }
        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_cachedAreaId, 4);
                ar->Write(&m_cachedSubId, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_cachedAreaId, 4);
                ar->Read(&m_cachedSubId, 4);
                break;
        }
        return 1;
    }
    RVA(0x00011d70, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMETEXT;
    }

public:
    CInGameText() {}
    CInGameText(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    i32 Update();

    i32 m_cachedAreaId;
    i32 m_cachedSubId;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CINGAMETEXT_H
