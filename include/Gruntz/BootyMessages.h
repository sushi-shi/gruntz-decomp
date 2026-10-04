#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GlyphStringDraw.h>

struct SecretMsgRow {
    char m_strA[0x20];
    char m_strB[0x80];
};

extern RECT g_bootyStatLabelRects[8];

extern CString g_bootyStatLabels[8];

class CString;

extern const Coord g_bootyLetterCoords[16];

extern const float g_secretRatioScale;

#endif // GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
