#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGridIter.h>
#include <Ints.h>

class CDDrawSurfaceMgr;

struct WwdSnapshot {
    i32 m_id;
    i32 m_objectId;
    LoadableClassId m_classId;
    i32 m_serialTypeId;
    LogicTypeId m_logicTypeId;
    char m_workerName[0x80];
    i32 m_screenX;
    i32 m_screenY;
    i32 m_sortKey;
};
SIZE(0xa0);

class CDDrawWorker;

class CImage;
class CDDrawSurfacePair;
struct LeafCue;

// @identity-TODO
// The live callback-object snapshot branch proves slot 16, but the concrete owner
// remains absent from RTTI, allocation sites, and the factory, whose mode-10 arm
// returns failure without constructing an object.
VTBL_ABSENT(CWwdGameObjectSerial);
class CWwdGameObjectSerial : public CGameObject {
public:
    virtual i32 GetSerialTypeId();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_WWDGAMEOBJECT_H
