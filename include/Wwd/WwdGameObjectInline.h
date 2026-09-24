#ifndef GRUNTZ_WWD_WWDGAMEOBJECTINLINE_H
#define GRUNTZ_WWD_WWDGAMEOBJECTINLINE_H

#include <AddrWord.h>
#include <DDrawMgr/LogicRecord.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

static inline i32 NotifyLogicForEventCode(CGameObject* object, i32 eventCode) {
    CLogicRecord* record = object->m_logicRecord;
    if (!record) {
        return 0;
    }
    i32 savedEventCode = record->m_eventCode;
    record->SetEventCode(eventCode);
    object->m_logicRecord->m_dispatch(object);
    if (object->m_logicRecord->m_eventCode == eventCode) {
        object->m_logicRecord->SetEventCode(savedEventCode);
    }
    return 1;
}

static inline BOOL LookupLinkedObject(CMapPtrToPtr& map, i32 id, CWwdGameObject*& out) {
    out = NULL;
    AddrWord<char> key;
    key.m_word = id;
    MapOutRef<CWwdGameObject> dst;
    dst.m_asTyped = &out;
    return map.Lookup(key.m_addr, *dst.m_asVoid);
}

#endif // GRUNTZ_WWD_WWDGAMEOBJECTINLINE_H
