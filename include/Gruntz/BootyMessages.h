#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

#include <string>

#include <Ints.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GlyphStringDraw.h>

struct SecretMsgRow {
    char m_strA[0x20];
    char m_strB[0x80];
};

extern RECT g_levelMsgRectsA[8];

extern std::string g_levelMsgStrings[8];


extern const Coord g_bootyLetterCoords[16];

extern const float g_secretRatioScale;

i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    std::string* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

#endif
