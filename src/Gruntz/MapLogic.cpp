#include <rva.h>

#include <Gruntz/MapLogic.h>

#include <Mfc.h>

#include <Gruntz/ScrollState.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

RVA(0x000ec230, 0x11c)
i32 MapSerializeCurve(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollAccum, sizeof(g_scrollAccum));
            ar->Write(&g_scrollLimit, sizeof(g_scrollLimit));
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollAccum, sizeof(g_scrollAccum));
            ar->Read(&g_scrollLimit, sizeof(g_scrollLimit));
            break;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollClock, sizeof(g_scrollClock));
            ar->Write(&g_scrollTimer, sizeof(g_scrollTimer));
            ar->Write(&g_scrollSave18, sizeof(g_scrollSave18));
            ar->Write(&g_scrollSave1c, sizeof(g_scrollSave1c));
            ar->Write(&g_lastScrollX, sizeof(g_lastScrollX));
            ar->Write(&g_lastScrollY, sizeof(g_lastScrollY));
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollClock, sizeof(g_scrollClock));
            ar->Read(&g_scrollTimer, sizeof(g_scrollTimer));
            ar->Read(&g_scrollSave18, sizeof(g_scrollSave18));
            ar->Read(&g_scrollSave1c, sizeof(g_scrollSave1c));
            ar->Read(&g_lastScrollX, sizeof(g_lastScrollX));
            ar->Read(&g_lastScrollY, sizeof(g_lastScrollY));
            break;
    }
    return 1;
}
