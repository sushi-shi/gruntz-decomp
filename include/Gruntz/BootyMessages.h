#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GlyphStringDraw.h>

struct SecretMsgRow {
    char strA[0x20];
    char strB[0x80];
};

extern RECT g_levelMsgRectsA[8];

extern CString g_levelMsgStrings[8];

class CString;

extern const Coord g_bootyLetterCoords[16];

extern const float g_secretRatioScale;

i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
