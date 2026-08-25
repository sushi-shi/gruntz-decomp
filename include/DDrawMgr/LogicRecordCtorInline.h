#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H

#include <rva.h>

#include <DDrawMgr/LogicRecord.h>

inline CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 logicFlags)
    : CWapObj(owner, id, logicFlags, CWapObj::NO_SEED) {
    ResetLogicFields();
}

#endif // GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H
