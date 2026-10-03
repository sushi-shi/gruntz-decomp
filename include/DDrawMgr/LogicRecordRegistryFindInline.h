#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRYFINDINLINE_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDREGISTRYFINDINLINE_H

#include <Utils/MapTyped.h>
#include <Ints.h>

#include <DDrawMgr/LogicRecordRegistry.h>

inline CLogicRecord* CLogicRecordRegistry::FindTemplate(const char* key) {
    CObject* found = NULL;
    ASSERT(key != NULL);
    MapLookup(m_templatesByName, key, found);
    return static_cast<CLogicRecord*>(found);
}

#endif
