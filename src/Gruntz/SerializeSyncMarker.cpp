#include <Mfc.h>                  // wsprintfA (USER32, reloc-masked)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Io/FileMem.h>           // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (g_gameReg->EnterModalUI)
#include <Gruntz/SerialArchive.h> // CFileMemBase (arc->Read @+0x2c / Write @+0x30)
#include <rva.h>

RVA(0x00013610, 0x8c)
i32 SerializeSyncMarker(CFileMemBase* arc, i32 mode, const char* name, i32 line) {
    if (mode == 4) {
        i32 marker = g_serialCounter + 0x1234666;
        arc->Write(&marker, 4);
        return 1;
    }
    if (mode == 7) {
        i32 readVal;
        arc->Read(&readVal, 4);
        if (readVal != g_serialCounter + 0x1234666) {
            wsprintfA(g_syncErrMsgBuf, "save/load out of sync at %s, %d", name, line);
            g_gameReg->EnterModalUI(g_syncErrMsgBuf);
            return 0;
        }
    }
    return 1;
}
