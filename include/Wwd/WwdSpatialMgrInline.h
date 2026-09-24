#ifndef GRUNTZ_WWD_WWDSPATIALMGRINLINE_H
#define GRUNTZ_WWD_WWDSPATIALMGRINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/WwdGameObject.h>
#include <Gruntz/WwdGrid.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdSpatialMgr.h>

inline i32 CWwdSpatialMgr::DeactivateRegionObject(
    CWwdGrid* grid,
    POSITION pos,
    CWwdGameObject* obj,
    WwdRegion* region,
    WwdGameObjectFlags flags
) {
    if (HAS(flags, WWD_GAME_OBJECT_FLAG_DELETE_ON_DEACTIVATE)) {
        if (HAS(flags, WWD_GAME_OBJECT_FLAG_DISPATCH_OBJECT_REMOVED)) {
            CLogicRecord* record = obj->m_logicRecord;
            record->SetLogicEvent(ACT_OBJECT_REMOVED);
            record->m_dispatch(obj);
        }
        m_activeGroup->RemoveAll(pos, obj);
        delete obj;
    } else {
        if (HAS(flags, WWD_GAME_OBJECT_FLAG_DISPATCH_LEAVE_ACTIVE_REGION)) {
            CLogicRecord* record = obj->m_logicRecord;
            i32 saved = record->EventCode();
            record->SetLogicEvent(ACT_LEAVE_ACTIVE_REGION);
            record->m_dispatch(obj);
            if (record->LogicEvent() == ACT_LEAVE_ACTIVE_REGION) {
                record->SetEventCode(saved);
            }
        }
        m_activeGroup->RemoveByPosition(pos, region->m_object);
        grid->Add(region);
    }
    return 1;
}

#endif // GRUNTZ_WWD_WWDSPATIALMGRINLINE_H
