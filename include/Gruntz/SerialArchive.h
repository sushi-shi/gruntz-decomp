#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

#include <Enums.h>

class CFileMemBase;

// The archive drives objects through THREE PHASES per direction, in this order:
//   save:    SERIAL_SNAPSHOT_BEGIN -> SERIAL_PRESAVE -> SERIAL_SAVE -> SERIAL_POSTSAVE
//   restore: SERIAL_RESTORE_BEGIN  -> SERIAL_PRELOAD -> SERIAL_LOAD -> SERIAL_POSTLOAD
// proven by CDDrawSurfaceMgr::SnapshotChildren / ::RestoreChildren, which call
// the child group, the callback and the level with 3,4,5 then 6,7,8.
// The pre/post phases are the pointer<->id swizzle: CGameObject's case 3 writes
// `m_carrierId = m_carrier->m_objectId`, and its case 8 looks the id back up
// with MapLookupById.
GZ_ENUM_BEGIN(SerialMode)
// The archive opens each direction with a BEGIN phase before the
// pre/main/post trio (CDDrawSurfaceMgr passes 1 then 3,4,5; 2 then 6,7,8).
    SERIAL_SNAPSHOT_BEGIN = 1,
    SERIAL_RESTORE_BEGIN = 2,
    SERIAL_PRESAVE = 3,
    SERIAL_SAVE = 4,
    SERIAL_POSTSAVE = 5,
    SERIAL_PRELOAD = 6,
    SERIAL_LOAD = 7,
    SERIAL_POSTLOAD = 8,

    // Construction callbacks: the archive asks the factory to make an
    // object of the given type id. SerialObjectFactory's arm 9 switches on
    // LOGIC_* and news the class; arm 10 declines (returns 0).
    SERIAL_CREATE = 9,
    SERIAL_CREATE_BY_SERIAL_ID = 10
GZ_ENUM_END(SerialMode)

extern "C" char g_syncErrMsgBuf[];

#endif // GRUNTZ_SERIALARCHIVE_H
