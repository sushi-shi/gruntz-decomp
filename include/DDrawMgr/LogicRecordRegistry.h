#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRY_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRY_H

#include <rva.h>

#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/MapStringToOb.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CLogicRecordRegistry : public CWapObj {
public:
    CLogicRecordRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}
    virtual ~CLogicRecordRegistry() OVERRIDE;

    RVA(0x001576d0, 0x16)
    virtual i32 IsLoaded() OVERRIDE {
        if (m_ownerCtx == NULL) {
            goto fail;
        }
        if (m_id != -1) {
            return 1;
        }

    fail:
        return 0;
    }

    RVA(0x00157790, 0x6)
    virtual i32 IsReady() OVERRIDE {
        return 1;
    }

    virtual void Unload() OVERRIDE;
    RVA(0x001576f0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_LOGICRECORDREGISTRY;
    }

    virtual CLogicRecord*
    RegisterLogicType(LogicRecordDispatchFn dispatch, const char* key, i32 flags);

    CLogicRecord* FindTemplate(const char* key);

    CString FindLogicTypeKey(CLogicRecord* record);

    CMapStringToOb m_templatesByName;
};

#endif // GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRY_H
