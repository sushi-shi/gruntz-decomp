#ifndef GRUNTZ_TEXTBOUNDS_H
#define GRUNTZ_TEXTBOUNDS_H

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>

#define GET_TEXT_BOUNDS(result, bounds)                                                            \
    i32 bottom = (bounds).bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");                     \
    i32 right = (bounds).right - g_buteMgr.GetInt("Font", "TextRightEdge");                        \
    i32 top = (bounds).top + g_buteMgr.GetInt("Font", "TextTopEdge");                              \
    i32 left = (bounds).left + g_buteMgr.GetInt("Font", "TextLeftEdge");                           \
    SetRect(&(result), left, top, right, bottom)

inline CRect GetTextBounds(const RECT& bounds) {
    CRect insets;
    insets.bottom = g_buteMgr.GetInt("Font", "TextBottomEdge");
    insets.right = g_buteMgr.GetInt("Font", "TextRightEdge");
    insets.top = g_buteMgr.GetInt("Font", "TextTopEdge");
    insets.left = g_buteMgr.GetInt("Font", "TextLeftEdge");
    CRect result(bounds);
    result.DeflateRect(&insets);
    return result;
}

#endif // GRUNTZ_TEXTBOUNDS_H
