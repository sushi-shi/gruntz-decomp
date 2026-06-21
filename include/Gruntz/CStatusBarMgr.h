// CStatusBarMgr.h - the in-game status-bar manager (C:\Proj\Gruntz).
//
// CStatusBarMgr owns the five status-bar "tabs" (Statz / Gruntz / Resource /
// Multiplayer / Game). Each tab is a list of CStatusBarItem-derived widgets
// (CSBI_RectOnly / CSBI_ImageSet / CSBI_Image / CSBI_SideTab / ...). The big
// per-tab builder LoadTabSprites() (RVA 0x102250) dispatches on the current tab
// index (m_10c) and, for the selected tab, creates each widget, stamps its
// vtable + type tag, configures it from a named sprite-asset key + a geometry
// CRect, and appends it to the tab's CPtrList.
//
// Only offsets + code bytes are load-bearing; field names are placeholders for
// the engine identities recovered from the member writes in the builder.
#ifndef GRUNTZ_CSTATUSBARMGR_H
#define GRUNTZ_CSTATUSBARMGR_H

#include <Gruntz/StatusBarItem.h>

// ---------------------------------------------------------------------------
// The geometry rectangle passed by value into the item "configure" virtuals.
// Built as base + per-item offsets: left/top/right/bottom = m_10 + dx / m_14 + dy.
// ---------------------------------------------------------------------------
struct SbRect {
    int left;
    int top;
    int right;
    int bottom;
};

// ---------------------------------------------------------------------------
// The created status-bar widgets. Each is a CStatusBarItem subtype; the builder
// stamps the retail vtable address directly (transitional workaround - the
// subclass virtuals point into other TUs, so the compiler cannot reproduce the
// vtable contents) and drives one of two configure virtuals:
//   Configure   = vtable slot +0x2c (key + rect + a handful of ints)
//   ConfigureEx = vtable slot +0x34 (the HEAD/ARROW variant: more ints)
// Both take the owning CStatusBarMgr as their first explicit arg, return an int
// (0 = failure -> the builder deletes the item and bails).
// ---------------------------------------------------------------------------
class CStatusBarMgr;

class CSBI : public CStatusBarItem {
public:
    virtual int Configure(CStatusBarMgr* mgr, int a, int b, int c, SbRect rect, const char* key,
                          int d, int e);
    virtual int ConfigureEx(CStatusBarMgr* mgr, int a0, SbRect rect, const char* key, int b, int c,
                            int d, int e, int f);
};

#endif // GRUNTZ_CSTATUSBARMGR_H
