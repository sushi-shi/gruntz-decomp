#include <Mfc.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <Gruntz/MapLogic.h>

#include <rva.h>

#include <Gruntz/ScrollState.h> // g_scrollAccum (bound in MgrAutoScroll.cpp)

RVA(0x000ec230, 0x11c)
i32 MapSerializeCurve(CSerialArchive* ar, i32 mode, i32, i32) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ar->Write(&g_scrollAccum, 8);
            ar->Write(&g_scrollLimit, 8);
            break;
        case 7:
            ar->Read(&g_scrollAccum, 8);
            ar->Read(&g_scrollLimit, 8);
            break;
    }
    switch (mode) {
        case 4:
            ar->Write(&g_scrollClock, 4);
            ar->Write(&g_scrollTimer, 4);
            ar->Write(reinterpret_cast<char*>(&g_scrollAccum) + 0x18, 4); // @identity-TODO (see ScrollState.h)
            ar->Write(reinterpret_cast<char*>(&g_scrollAccum) + 0x1c, 4); // @identity-TODO
            ar->Write(&g_lastScrollX, 4);
            ar->Write(&g_lastScrollY, 4);
            break;
        case 7:
            ar->Read(&g_scrollClock, 4);
            ar->Read(&g_scrollTimer, 4);
            ar->Read(reinterpret_cast<char*>(&g_scrollAccum) + 0x18, 4); // @identity-TODO (see ScrollState.h)
            ar->Read(reinterpret_cast<char*>(&g_scrollAccum) + 0x1c, 4); // @identity-TODO
            ar->Read(&g_lastScrollX, 4);
            ar->Read(&g_lastScrollY, 4);
            break;
    }
    return 1;
}
