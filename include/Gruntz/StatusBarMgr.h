// StatusBarMgr.h - the in-game status-bar manager (C:\Proj\Gruntz).
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

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>           // CPtrList (the five embedded tab lists)
#include <Gruntz/SbRect.h> // the geometry rect passed by value into the configure virtuals
#include <Gruntz/StatusBarItem.h>

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
    virtual i32
    Configure(CStatusBarMgr* mgr, i32 a, i32 b, i32 c, SbRect rect, const char* key, i32 d, i32 e);
    virtual i32 ConfigureEx(
        CStatusBarMgr* mgr,
        i32 a0,
        SbRect rect,
        const char* key,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f
    );
};
SIZE_UNKNOWN(CSBI);

// CStatusBarMgr layout (placeholder fields; only offsets are load-bearing). Moved
// here from <Gruntz/StatusBarMgrBuilders.h> (its natural home) so non-builder TUs
// can reach LoadTabSprites without the builder-facet SBI leaf views.
class CStatusBarMgr {
public:
    i32 LoadTabSprites(); // 0x102250
    void BuildGameMenu(); // 0x101580 (the GAMETAB menu builder; called at the Game-tab tail)

    i32 m_00; // +0x00  status-bar side/mode selector (Statz arrow x-span + arrow dir gate)
    char m_pad04[0xc - 4];
    i32 m_code; // +0x0c  the Configure `code` arg (== BuildGameMenu m_code)
    i32 m_10;   // +0x10  base x
    i32 m_14;   // +0x14  base y
    char m_pad18[0x48 - 0x18];
    CPtrList m_48; // +0x48  Statz tab list
    CPtrList m_64; // +0x64  Gruntz tab list
    CPtrList m_80; // +0x80  Resource tab list
    CPtrList m_9c; // +0x9c  Multiplayer tab list
    CPtrList m_b8; // +0xb8  Game tab list
    char m_padd4[0x10c - 0xd4];
    i32 m_10c; // +0x10c  current tab selector (1..5)
    // ----- created-widget cache slots (only offsets are load-bearing) -----
    char m_pad110[0x114 - 0x110];
    i32 m_114; // +0x114  Statz smallicon cache base
    char m_pad118[0x204 - 0x118];
    i32 m_204; // +0x204  Gruntz GRUNTOVEN item cache (5, stride 4)
    char m_pad208[0x218 - 0x208];
    i32 m_218; // +0x218  Gruntz OVENZTEXT slot
    i32 m_21c; // +0x21c  Gruntz WELLGOO slot
    char m_pad220[0x224 - 0x220];
    i32 m_224; // +0x224  Gruntz GRUNTOVEN format cache (5, stride 0x18)
    char m_pad228[0x298 - 0x228];
    i32 m_298; // +0x298  Gruntz WELLGOO config-d source
    char m_pad29c[0x2c4 - 0x29c];
    i32 m_2c4; // +0x2c4  Resource BELT config-d source (3, stride 0x18)
    char m_pad2c8[0x2dc - 0x2c8];
    i32 m_2dc; // +0x2dc  (BELT[1])
    char m_pad2e0[0x2f4 - 0x2e0];
    i32 m_2f4; // +0x2f4  (BELT[2])
    char m_pad2f8[0x308 - 0x2f8];
    i32 m_308; // +0x308  Resource BELT item cache (3, stride 4)
    i32 m_30c;
    i32 m_310;
    char m_pad314[0x31c - 0x314];
    i32 m_31c; // +0x31c  Resource MACHINE cross-call idxB source
    char m_pad320[0x334 - 0x320];
    i32 m_334; // +0x334  Resource MACHINE cross-call idxA source
    char m_pad338[0x348 - 0x338];
    i32 m_348; // +0x348  Resource MACHINE item slot
    char m_pad34c[0x364 - 0x34c];
    i32 m_364; // +0x364  Resource MAINBG slot
    i32 m_368; // +0x368  (multiplayer slot base)
    i32 m_36c; // +0x36c  Resource UPPERBG slot
    i32 m_370; // +0x370  Resource WINDOWBG slot
    char m_pad374[0x3dc - 0x374];
    i32 m_3dc; // +0x3dc  Resource SHREDDER config-d array
    char m_pad3e0[0x4a8 - 0x3e0];
    i32 m_4a8; // +0x4a8  Resource SHREDDER item cache
    char m_pad4ac[0x4cc - 0x4ac];
    i32 m_4cc; // +0x4cc  Resource MACHINE config-d source
    char m_pad4d0[0x4e0 - 0x4d0];
    i32 m_4e0; // +0x4e0  Resource MACHINE-shredder item slot
    char m_pad4e4[0x4ec - 0x4e4];
    i32 m_4ec; // +0x4ec  Multiplayer config-d source
    char m_pad4f0[0x500 - 0x4f0];
    i32 m_500; // +0x500  Multiplayer shredder item slot
    i32 m_504; // +0x504  rect-cached x
    i32 m_508; // +0x508  rect-cached y
    i32 m_50c; // +0x50c  rect-cached
    i32 m_510; // +0x510  rect-cached
    i32 m_514; // +0x514  Resource MACHINE rect-cached x
    i32 m_518; // +0x518  rect-cached y
    i32 m_51c; // +0x51c  rect-cached
    i32 m_520; // +0x520  rect-cached
    char m_pad524[0x61c - 0x524];
    i32 m_61c; // +0x61c  Multiplayer player-slot cache (4, stride 4)
    i32 m_620;
    i32 m_624;
    i32 m_628;
    i32 m_62c; // +0x62c  Multiplayer head slot
};
SIZE_UNKNOWN(CStatusBarMgr);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CSTATUSBARMGR_H
