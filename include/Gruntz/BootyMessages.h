#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#include <rva.h>
#include <Gruntz/GlyphStringDraw.h> // RECT (for the extern below)

struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
SIZE_UNKNOWN();

extern RECT g_levelMsgRectsA[8]; // 0x60b838  (shared with BootyMessages - stays extern)

extern CString g_levelMsgStrings[8]; // 0x00229ef8

class CString;
// CORRECTED 2026-07-29: the trailing six were spelled (y, flag, b, g, r, a9), which is
// the EngStr_DrawText / ShowHudMessage family's slot set written backwards. It IS that
// family - same nine-slot shape, and all five call sites in BootyStateActivate.cpp pass
// (m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1), i.e. font 0x78, shadow on, yellow.
extern "C" void DrawStatText(
    void* ctx,
    CString* text,
    RECT* rc,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
); // (booty stat text row)

// 0x001e8fe8, .rdata, 0x80 B = 32 i32 (gap to g_idleSpriteIds @0x1e9068). Layout:
// four (x,y) anchor pairs, then three identical rows of eight column x-positions.
extern "C" const i32 g_bootyLetterCoords[32];

extern float g_secretRatioScale;
extern char g_secretMsgA[0x20];
extern char g_secretMsgB[0x80];

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0x1154b0

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
