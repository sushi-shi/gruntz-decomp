#ifndef GRUNTZ_STATUSBARHIGHLIGHTROW_H
#define GRUNTZ_STATUSBARHIGHLIGHTROW_H

#include <Enums.h>

// Row selected within each four-entry status-bar highlight group. The keypad
// dispatch maps row 0 to the group/category key and rows 1..3 to upper, middle,
// and lower directional keys.
GZ_ENUM_BEGIN(StatusBarHighlightRow)
    STATUS_HL_ROW_NONE = -1,
    STATUS_HL_ROW_CATEGORY = 0,
    STATUS_HL_ROW_UPPER = 1,
    STATUS_HL_ROW_MIDDLE = 2,
    STATUS_HL_ROW_LOWER = 3
GZ_ENUM_END(StatusBarHighlightRow)

#endif // GRUNTZ_STATUSBARHIGHLIGHTROW_H
