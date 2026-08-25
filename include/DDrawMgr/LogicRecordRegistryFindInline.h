#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRYFINDINLINE_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRYFINDINLINE_H

#include <rva.h>

#include <DDrawMgr/LogicRecordRegistry.h>

inline CLogicRecord* CLogicRecordRegistry::FindTemplate(const char* key) {
    CObject* found = NULL;
    ASSERT(key != NULL);
    m_templatesByName.Lookup(key, found);
    return static_cast<CLogicRecord*>(found);
}

#endif // GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRYFINDINLINE_H
