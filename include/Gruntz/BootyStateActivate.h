#ifndef GRUNTZ_BOOTYSTATEACTIVATE_H
#define GRUNTZ_BOOTYSTATEACTIVATE_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

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
);

#endif // GRUNTZ_BOOTYSTATEACTIVATE_H
