#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

#include <rva.h>

#include <Gruntz/GlyphStringDraw.h>

struct SecretMsgRow {
    char strA[0x20];
    char strB[0x80];
};

extern RECT g_levelMsgRectsA[8];

extern CString g_levelMsgStrings[8];

class CString;

extern const i32 g_bootyLetterCoords[32];

extern const float g_secretRatioScale;
extern char g_secretMsgA[0x20];
extern char g_secretMsgB[0x80];

i32 ShowHudMessage(
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

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
