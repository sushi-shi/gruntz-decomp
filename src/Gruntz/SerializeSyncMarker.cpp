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

RVA(0x00013610, 0x8c)
i32 SerializeSyncMarker(CFileMemBase* arc, i32 mode, const char* name, i32 line) {
    if (mode == 4) {
        i32 marker = g_serialCounter + 0x1234666;
        arc->Write(&marker, sizeof(marker));
        return 1;
    }
    if (mode == 7) {
        i32 readVal;
        arc->Read(&readVal, sizeof(readVal));
        if (readVal != g_serialCounter + 0x1234666) {
            wsprintfA(g_syncErrMsgBuf, "save/load out of sync at %s, %d", name, line);
            g_gameReg->EnterModalUI(g_syncErrMsgBuf);
            return 0;
        }
    }
    return 1;
}
