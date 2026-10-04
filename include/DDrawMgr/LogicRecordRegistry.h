#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRY_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRY_H

#include <map>
#include <string>


#include <Ints.h>

#include <DDrawMgr/LogicRecord.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CLogicRecordRegistry : public CWapObj {
public:
    CLogicRecordRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}
    virtual ~CLogicRecordRegistry()  ;

    virtual i32 IsLoaded()   {
        if (m_ownerCtx == NULL) {
            goto fail;
        }
        if (m_id != -1) {
            return 1;
        }

    fail:
        return 0;
    }

    virtual i32 IsReady()   {
        return 1;
    }

    virtual void Unload()  ;

    virtual LoadableClassId GetClassId()   {
        return CLASSID_LOGICRECORDREGISTRY;
    }

    virtual CLogicRecord*
    RegisterLogicType(LogicRecordDispatchFn dispatch, const std::string& key, i32 flags);

    CLogicRecord* FindTemplate(const std::string& key);

    std::string FindLogicTypeKey(CLogicRecord* record);

private:
    std::map<std::string, CLogicRecord*> m_templatesByName;
};

#endif
