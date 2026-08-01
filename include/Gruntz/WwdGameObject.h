#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/WwdGridIter.h>
#include <DDrawMgr/AnimWorkerObj.h>

class CDDrawSurfaceMgr;

struct WwdSnapshot {
    i32 m_00;
    i32 m_04;
    i32 m_08;

    i32 m_serialTypeId;
    i32 m_10;
    char m_name[0x80];
    i32 m_94;
    i32 m_98;
    i32 m_9c;
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
