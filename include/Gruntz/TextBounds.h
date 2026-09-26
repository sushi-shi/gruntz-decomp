#ifndef GRUNTZ_GRUNTZ_TEXTBOUNDS_H
#define GRUNTZ_GRUNTZ_TEXTBOUNDS_H

#include <Bute/ButeMgr.h>

#define GET_TEXT_BOUNDS(result, bounds)                                                            \
    i32 bottom = (bounds).bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");                     \
    i32 right = (bounds).right - g_buteMgr.GetInt("Font", "TextRightEdge");                        \
    i32 top = (bounds).top + g_buteMgr.GetInt("Font", "TextTopEdge");                              \
    i32 left = (bounds).left + g_buteMgr.GetInt("Font", "TextLeftEdge");                           \
    SetRect(&(result), left, top, right, bottom)

#endif // GRUNTZ_GRUNTZ_TEXTBOUNDS_H
