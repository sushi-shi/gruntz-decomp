#include <rva.h>

#include <Gruntz/MapLogic.h>

#include <Mfc.h>

#include <Gruntz/ScrollState.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

RVA(0x000ec230, 0x11c)
i32 MapSerializeCurve(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollAccum, 8);
            ar->Write(&g_scrollLimit, 8);
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollAccum, 8);
            ar->Read(&g_scrollLimit, 8);
            break;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollClock, 4);
            ar->Write(&g_scrollTimer, 4);
            ar->Write(&g_scrollSave18, 4);
            ar->Write(&g_scrollSave1c, 4);
            ar->Write(&g_lastScrollX, 4);
            ar->Write(&g_lastScrollY, 4);
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollClock, 4);
            ar->Read(&g_scrollTimer, 4);
            ar->Read(&g_scrollSave18, 4);
            ar->Read(&g_scrollSave1c, 4);
            ar->Read(&g_lastScrollX, 4);
            ar->Read(&g_lastScrollY, 4);
            break;
    }
    return 1;
}
