#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Io/FileMem.h>

DATA(0x00229a50)
char g_syncErrMsgBuf[0x80];
DATA(0x00229ad0)
i32 g_serialCounter;

// The base of the save/load sync marker. Nothing decodes it - the writer emits
// g_serialCounter + this and the reader recomputes the same sum - so its only
// job is to be a value a stray stream position is unlikely to hold. Both sites
// spell it out; naming it is what makes the pair obviously one constant.
GZ_ENUM_CONST_BEGIN(SerialSyncMarker)
    SERIAL_SYNC_MARKER_BASE = 0x1234666
GZ_ENUM_CONST_END(SerialSyncMarker)

RVA(0x00013610, 0x8c)
i32 SerializeSyncMarker(CFileMemBase* arc, i32 mode, const char* name, i32 line) {
    if (mode == SERIAL_SAVE) {
        i32 marker = g_serialCounter + SERIAL_SYNC_MARKER_BASE;
        arc->Write(&marker, sizeof(marker));
        return 1;
    }
    if (mode == SERIAL_LOAD) {
        i32 readVal;
        arc->Read(&readVal, sizeof(readVal));
        if (readVal != g_serialCounter + SERIAL_SYNC_MARKER_BASE) {
            wsprintfA(g_syncErrMsgBuf, "save/load out of sync at %s, %d", name, line);
            g_gameReg->EnterModalUI(g_syncErrMsgBuf);
            return 0;
        }
    }
    return 1;
}
