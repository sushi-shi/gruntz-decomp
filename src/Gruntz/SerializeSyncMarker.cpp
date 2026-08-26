#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialCounter.h>
#include <Io/FileMem.h>

DATA(0x0022a9a8)
char g_syncErrMsgBuf[0x80];
DATA(0x0022aa28)
i32 g_serialCounter;

GZ_ENUM_CONST_BEGIN(SerialSyncMarker)
    SERIAL_SYNC_MARKER_BASE = 0x1234666
GZ_ENUM_CONST_END(SerialSyncMarker)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00013620, 0x8c)
i32 SerializeSyncMarker(CFileMemBase* arc, i32 mode, const char* name, i32 line) {
    SerialMode serialMode = static_cast<SerialMode>(mode);
    if (serialMode == SERIAL_SAVE) {
        i32 marker = g_serialCounter + SERIAL_SYNC_MARKER_BASE;
        arc->Write(&marker, sizeof(marker));
        return 1;
    }
    if (serialMode == SERIAL_LOAD) {
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
