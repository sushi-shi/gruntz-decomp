#ifndef GRUNTZ_GRUNTZ_COLORTINT_H
#define GRUNTZ_GRUNTZ_COLORTINT_H

#include <Mfc.h>

#include <Enums.h>

// The 17 team/tool colour variants. Names are retail's own sprite-namespace
// strings (BLACK BLUE CYAN DKBLUE DKGREEN DKRED DKYELLOW GREEN GREY HOTPINK
// ORANGE PINK PURPLE RED TURQ WHITE YELLOW - docs/strings-analysis.md); the
// ORDER is proven, not guessed, by the six identical index -> COLORREF switches
// in Dialogs.cpp / MultiStartDlgRoster.cpp / BootyStateActivate.cpp. Reading
// each arm as 0x00BBGGRR:
//
//   0 ff8000 orange   1 00ff00 green    2 0000ff blue     3 ff0000 red
//   4 800080 purple   5 ffff00 yellow   6 ff0080 hotpink  7 (default) black
//   8 000080 dkblue   9 008000 dkgreen 10 008080 turq    11 800000 dkred
//  12 ff00ff pink    13 808000 dkyellow 14 808080 grey   15 00ffff cyan
//  16 ffffff white
//
// An earlier alphabetical guess at this order was wrong (it put GREY at 8) and
// was deleted; this is the measured one.
GZ_ENUM_BEGIN_SPLIT(ColorTint, u8)
    TINT_ORANGE = 0,
    TINT_GREEN = 1,
    TINT_BLUE = 2,
    TINT_RED = 3,
    TINT_PURPLE = 4,
    TINT_YELLOW = 5,
    TINT_HOTPINK = 6,
    TINT_BLACK = 7,
    TINT_DKBLUE = 8,
    TINT_DKGREEN = 9,
    TINT_TURQ = 10,
    TINT_DKRED = 11,
    TINT_PINK = 12,
    TINT_DKYELLOW = 13,
    TINT_GREY = 14,
    TINT_CYAN = 15,
    TINT_WHITE = 16,
    TINT_COUNT = 17
GZ_ENUM_END_SPLIT(ColorTint, u8)

#endif // GRUNTZ_GRUNTZ_COLORTINT_H
