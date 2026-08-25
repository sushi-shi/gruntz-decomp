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
// The fixed width of a name field in the save format.
//
// One fact spelled two ways, sometimes in adjacent lines - CWarlord serialises
// its name as `memset(buf, 0, sizeof(buf)); strcpy(buf, m_warlordName);
// ar->Write(buf, 0x80);`, where the memset already asks the buffer and the
// Write does not. Every one of the ~140 Read/Write pairs in the tree uses this
// same width, over buffers declared `char x[SERIAL_NAME_LEN]`, so the array
// bound and the byte count are the same number by construction.
//
// The width is LOAD-BEARING: it is a file-format field, not a buffer size that
// could be enlarged.
GZ_ENUM_CONST_BEGIN(SerialNameField)
    SERIAL_NAME_LEN = 0x80
GZ_ENUM_CONST_END(SerialNameField)

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
    // object of the given type id. GameSerializationCallback's arm 9 switches on
    // LOGIC_* and news the class; arm 10 declines (returns 0).
    SERIAL_CREATE = 9,
    SERIAL_CREATE_BY_SERIAL_ID = 10
GZ_ENUM_END(SerialMode)

extern char g_syncErrMsgBuf[];

#endif // GRUNTZ_SERIALARCHIVE_H
