// BootyStateActivate.h - the BootyStateActivate TU's external declarations.
#ifndef GRUNTZ_BOOTYSTATEACTIVATE_H
#define GRUNTZ_BOOTYSTATEACTIVATE_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr;

void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0x115520

#endif // GRUNTZ_BOOTYSTATEACTIVATE_H
