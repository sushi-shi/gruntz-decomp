#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#include <rva.h>

struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
SIZE_UNKNOWN();

#include <Gruntz/GlyphStringDraw.h> // RECT (for the extern below)
extern RECT g_levelMsgRectsA[8]; // 0x60b838  (shared with BootyMessages - stays extern)

extern CString g_levelMsgStrings[8]; // 0x00229ef8

class CString;
extern "C" void DrawStatText(void* ctx, CString* text, RECT* rc, i32 y, i32 flag,
                            i32 b, i32 g, i32 r, i32 a9); // (booty stat text row)

extern "C" i32 g_bootyLetterCoords[]; // 0x001e8fe8 (bound by DATA_SYMBOL in BootyStateActivate.cpp)

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
