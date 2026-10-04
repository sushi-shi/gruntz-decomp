#ifndef GRUNTZ_GRUNTZ_BOOTYMESSAGES_H
#define GRUNTZ_GRUNTZ_BOOTYMESSAGES_H

#include <string>
#include <Gruntz/GlyphStringDraw.h>

#include <Ints.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GlyphStringDraw.h>

struct SecretMsgRow {
    std::string m_strA;
    std::string m_strB;
};

extern RECT g_levelMsgRectsA[8];

extern std::string g_levelMsgStrings[8];


extern const Coord g_bootyLetterCoords[16];

extern const float g_secretRatioScale;


#endif
