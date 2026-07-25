#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#include <rva.h>
#include <Gruntz/GlyphStringDraw.h> // RECT (for the extern below)

struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
SIZE_UNKNOWN();

extern RECT g_levelMsgRectsA[8];    // 0x60b838  (shared with BootyMessages - stays extern)

extern CString g_levelMsgStrings[8]; // 0x00229ef8

class CString;
extern "C" void DrawStatText(
    void* ctx,
    CString* text,
    RECT* rc,
    i32 y,
    i32 flag,
    i32 b,
    i32 g,
    i32 r,
    i32 a9
); // (booty stat text row)

extern "C" i32 g_bootyLetterCoords[]; // 0x001e8fe8 (bound by DATA_SYMBOL in BootyStateActivate.cpp)

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
