#ifndef GRUNTZ_WWD_ANIMWORKERACT_H
#define GRUNTZ_WWD_ANIMWORKERACT_H

#include <Enums.h>

// The act code an animation worker dispatches on (CDDrawWorker::ActKey()).
//
// Every value is named by the handler its arm calls, so nothing is inferred -
// the dispatch tables across thirteen files all forward to the same six
// CUserLogic entry points:
//
//   0x1d -> OnObjectRemoved()          0x51 -> AfterSave()
//   0x1e -> OnLeaveActiveRegion()      0x52 -> AfterLoad()
//   0x50 -> PrepareSave()              0x53 -> AfterLoadReferences()
//
// The 0x50..0x53 run is the serialisation quartet in the order it is used:
// prepare, save, load, then fix up references once every object exists - which
// is the same shape as SerialMode's PRESAVE/SAVE/POSTSAVE ladder.
GZ_ENUM_BEGIN(AnimWorkerAct)
// The worker has no logic yet. Its arm is the same in all thirteen files:
// construct the CUserLogic subclass, Activate() it, hang it on m_logic, and
// set the act key to ACT_LIVE.
    ACT_UNINITIALISED = 0,
    // The logic exists and is running; the arm is empty everywhere.
    ACT_LIVE = 0x3e8,
    ACT_OBJECT_REMOVED = 0x1d,
    ACT_LEAVE_ACTIVE_REGION = 0x1e,
    ACT_PREPARE_SAVE = 0x50,
    ACT_AFTER_SAVE = 0x51,
    ACT_AFTER_LOAD = 0x52,
    ACT_AFTER_LOAD_REFERENCES = 0x53
GZ_ENUM_END(AnimWorkerAct)

#endif // GRUNTZ_WWD_ANIMWORKERACT_H
