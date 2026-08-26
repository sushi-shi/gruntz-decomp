#include <rva.h>

#include <Gruntz/MapLogic.h>

#include <Mfc.h>

#include <Gruntz/ScrollState.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

RVA(0x000ec360, 0x11c)
i32 SerializeScrollState(CFileMemBase* ar, SerialMode mode, LogicTypeId, i32) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollPace.m_lastTime, sizeof(g_scrollPace.m_lastTime));
            ar->Write(&g_scrollPace.m_period, sizeof(g_scrollPace.m_period));
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollPace.m_lastTime, sizeof(g_scrollPace.m_lastTime));
            ar->Read(&g_scrollPace.m_period, sizeof(g_scrollPace.m_period));
            break;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&g_scrollClock, sizeof(g_scrollClock));
            ar->Write(&g_scrollTimer, sizeof(g_scrollTimer));
            ar->Write(&g_serializedScrollReservedFirst, sizeof(g_serializedScrollReservedFirst));
            ar->Write(&g_serializedScrollReservedSecond, sizeof(g_serializedScrollReservedSecond));
            ar->Write(&g_lastScrollX, sizeof(g_lastScrollX));
            ar->Write(&g_lastScrollY, sizeof(g_lastScrollY));
            break;
        case SERIAL_LOAD:
            ar->Read(&g_scrollClock, sizeof(g_scrollClock));
            ar->Read(&g_scrollTimer, sizeof(g_scrollTimer));
            ar->Read(&g_serializedScrollReservedFirst, sizeof(g_serializedScrollReservedFirst));
            ar->Read(&g_serializedScrollReservedSecond, sizeof(g_serializedScrollReservedSecond));
            ar->Read(&g_lastScrollX, sizeof(g_lastScrollX));
            ar->Read(&g_lastScrollY, sizeof(g_lastScrollY));
            break;
    }
    return 1;
}
