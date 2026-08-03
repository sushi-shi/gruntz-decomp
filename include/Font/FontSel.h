#ifndef FONT_FONTSEL_H
#define FONT_FONTSEL_H

#include <Enums.h>

// Which of the four loaded fonts a text call draws with.
//
// Each value names itself: EngStr_RenderText's switch does nothing per arm but
// hand one named global to g_textObj.SetFont.
//
//   100  g_tinyFont
//   110  g_smallFont
//   120  g_mediumFont
//   130  g_largeFont
//
// The spacing is the tell that these are a deliberate ladder rather than four
// unrelated tags - they step by ten from a round base, leaving room between
// sizes, and they sort in the order the globals are named.
GZ_ENUM_BEGIN(FontSel)
    FONTSEL_TINY = 100,
    FONTSEL_SMALL = 110,
    FONTSEL_MEDIUM = 120,
    FONTSEL_LARGE = 130
GZ_ENUM_END(FontSel)

#endif // FONT_FONTSEL_H
