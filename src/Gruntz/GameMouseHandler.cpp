#include <Mfc.h>
#include <Gruntz/TriggerMgr.h>
// GameMouseHandler.cpp - the in-game pointer/click dispatcher (C:\Proj\Gruntz),
// re-homed from src/Stub/Backlog.cpp (mis-labeled there as StatusBarItem::vfunc_16).
//
// Vtable-proven: 0xce660 is slot 16 (+0x40) of the CPlay / CMulti / CDemo state
// vtables (identical entry - a base CPlay virtual inherited by CMulti/CDemo), NOT a
// StatusBarItem method. It is the mouse sibling of the keyboard dispatcher in
// GameKeyHandler.cpp (adjacent RVA, same >0x4fc-byte PLAY-state object). The
// status-bar sub-objects it reaches (the active tab child m_2dc, the sound set, the
// area probe, the spawner) are modeled minimally; only offsets / code bytes are
// load-bearing and every helper is a reloc-masked external.
// <Gruntz/Play.h> (canonical CPlay) pulls <Mfc.h> -> <windows.h> the afx-first
// way, giving SetRect / RECT / POINT; a bare <Win32.h> here would make MFC hard-
// error ("MFC apps must not #include <windows.h>").
#include <Gruntz/Play.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/ChatBoxOwner.h> // canonical CChatBoxOwner (Configure)

#include <rva.h>

SIZE_UNKNOWN(SbiSndTable);
struct SbiSndTable {
    // Find @0x1b8438 IS CMapStringToOb::Lookup; cast at the call.
};
SIZE_UNKNOWN(SbiSndSet);
struct SbiSndSet { // m_4->m_30->m_28
    char m_pad00[0x10];
    SbiSndTable m_10; // +0x10
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};
SIZE_UNKNOWN(SbiPoint);
struct SbiPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN(SbiCoordSrc);
struct SbiCoordSrc { // m_4->m_30->m_24->m_5c
    char m_pad00[0x40];
    SbiPoint m_40; // +0x40
};
SIZE_UNKNOWN(SbiHost24);
struct SbiHost24 { // m_4->m_30->m_24
    char m_pad00[0x10];
    i32 m_10; // +0x10  origin x offset
    i32 m_14; // +0x14  origin y offset
    char m_pad18[0x5c - 0x18];
    SbiCoordSrc* m_5c; // +0x5c
};
SIZE_UNKNOWN(SbiHostInner);
struct SbiHostInner { // m_4->m_30
    char m_pad00[0x24];
    SbiHost24* m_24; // +0x24
    SbiSndSet* m_28; // +0x28
};
SIZE_UNKNOWN(SbiHost);
struct SbiHost { // this->m_4
    char m_pad00[0x30];
    SbiHostInner* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    CTriggerMgr* m_68;   // +0x68
    CGruntzCmdMgr* m_6c; // +0x6c
};
SIZE_UNKNOWN(SbiRectSrc);
struct SbiRectSrc { // this->m_c->m_24
    char m_pad00[0x10];
    RECT m_10; // +0x10
};
SIZE_UNKNOWN(SbiRectHost);
struct SbiRectHost { // this->m_c
    char m_pad00[0x24];
    SbiRectSrc* m_24; // +0x24
};
SIZE_UNKNOWN(SbiChild);
struct SbiChild { // this->m_2dc
    i32 m_0;      // +0x00  mode tag
};
SIZE_UNKNOWN(SbiEntry);
struct SbiEntry { // this->m_374[i]
    i32 m_0;      // +0x00  center x
    i32 m_4;      // +0x04  center y
};
SIZE_UNKNOWN(SbiMgr68);
struct SbiMgr68 { // g_sbiMgr->m_68
    char m_pad00[0x10c];
    i32 m_10c[1]; // +0x10c  per-area counter array
    char m_pad110[0x400 - 0x110];
    i32 m_400; // +0x400  active gate
};
SIZE_UNKNOWN(SbiCfgEntry);
struct SbiCfgEntry { // g_sbiMgr->m_150[area]  (stride 0x238)
    char m_pad00[0x228];
    i32 m_228; // +0x228  max count
    char m_pad22c[0x238 - 0x22c];
};
SIZE_UNKNOWN(SbiMgr);
struct SbiMgr { // g_sbiMgr (*0x64556c)
    char m_pad00[0x68];
    SbiMgr68* m_68; // +0x68
    char m_pad6c[0x150 - 0x6c];
    SbiCfgEntry m_150[1]; // +0x150
};
DATA(0x0024556c)
extern SbiMgr* g_sbiMgr; // *0x64556c
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA
DATA(0x00244c54)
extern "C" i32 g_644c54; // current area index
// sub_1c44 (0x1c44 thunk) __stdcall: is the hot point inside the mode-2 child?
i32 __stdcall SbiPointInChild(i32 x, i32 y);

// The CPlay members this dispatcher reaches map onto the canonical CState/CPlay
// layout, cast to this TU's status-bar facet views: m_4 (CState owner) -> SbiHost,
// m_c (CSpriteFactoryHolder) -> SbiRectHost, m_guts (+0x2dc) -> SbiChild, m_hitTest (+0x2e0) ->
// SbiToggle, m_markerData (+0x374) -> SbiEntry**. The base-handler at vtable slot
// +0x38 (CState::Vslot0e) is now a real virtual call.

// @early-stop
// 4-byte stack-coalesce wall (~84%, body byte-exact). The prior park (~52%) blamed
// only the frame, but the REAL blocker was return-epilogue tail-merge: retail funnels
// the two `return 1` guards to ONE shared epilogue and the two `return Dispatch()`
// guards to another, while a per-guard `if(a)return; if(b)return;` INLINES each
// epilogue. Fix: combine each paired guard with `||` (`if(m_484||!m_2dc)return 1;`
// short-circuits to the shared return with two tests + one exit) -> 51.7 -> 84.2.
// Residual is a uniform +4 frame shift (sub esp,0x14 vs 0x10): retail packs every
// local into the 16-byte RECT + path-dependent reuse of the dead x/y arg homes for
// the Probe/Find out-params; this build spills one out-param to a fresh 5th slot.
// A documented stack-slot-coalesce coin-flip (docs/patterns/stack-slot-coalesce-
// frame-4b.md), not source-steerable; ZERO logic differences remain.
RVA(0x000ce660, 0x362)
i32 CPlay::HandleMousePress(i32 msg, i32 x, i32 y) {
    if (m_hudSuppressed != 0 || m_guts == 0) {
        return 1;
    }
    if (m_overlayDrag != 0 || g_sbiMgr->m_68->m_400 == 0) {
        return ((CSBI_RectOnly*)m_guts)->ClickHilite(msg, x, y);
    }
    if (m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
        return this->Vslot0e(msg, x, y); // base handler at vtable slot +0x38
    }

    if (((SbiChild*)m_guts)->m_0 == 2 && SbiPointInChild(x, y)) {
        SbiSndSet* set = ((SbiHost*)m_4)->m_30->m_28;
        if (set->m_30 == 0) {
            LeafCue* e = 0;
            ((CMapStringToOb*)&set->m_10)->Lookup("GAME_TABHIGHLIGHT1", (CObject*&)e);
            if (e != 0) {
                e->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);
            }
        }
        ((CSBI_RectOnly*)m_guts)->RefreshState();
        if (((SbiChild*)m_guts)->m_0 == 1) {
            m_hitTest->Configure(2);
        } else {
            m_hitTest->Configure(1);
        }
        return 1;
    }

    i32 idx = ((CSBI_RectOnly*)m_guts)->HitTest(x, y);
    if (idx != -1) {
        ((CSBI_RectOnly*)m_guts)->PlaceCursorTarget(idx, 1);
        return 1;
    }

    RECT* rc = &((SbiRectHost*)m_c)->m_24->m_10;
    i32 rl = rc->left;
    i32 rt = rc->top;
    i32 rr = rc->right;
    i32 rb = rc->bottom;
    if (x < rl || x > rr || y < rt || y > rb) {
        return ((CSBI_RectOnly*)m_guts)->ClickHilite(msg, x, y);
    }

    i32 outArea;
    i32 outVal;
    if (((SbiHost*)m_4)->m_68->ScreenToCell(x, y, &outArea, &outVal, 5) && g_644c54 == outArea) {
        ((CSBI_RectOnly*)m_guts)->ToggleStat(outVal);
        return 1;
    }

    if (m_dragInhibit1 != 0) {
        return 1;
    }
    i32 area = g_644c54;
    SbiCfgEntry* cfg = &g_sbiMgr->m_150[area];
    if (cfg == 0) {
        return 0;
    }
    if (g_sbiMgr->m_68->m_10c[area] >= cfg->m_228) {
        return 0;
    }

    SbiHost24* h = ((SbiHost*)m_4)->m_30->m_24;
    i32 px = h->m_5c->m_40.x - h->m_10 + x;
    i32 py = h->m_5c->m_40.y - h->m_14 + y;
    for (i32 i = 0; i < markerCount(); i++) {
        SbiEntry* e = ((SbiEntry**)markerData())[i];
        if (e == 0) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_0 - 0x10, e->m_4 - 0x10, e->m_0 + 0x10, e->m_4 + 0x10);
        if (px < er.right && px >= er.left && py < er.bottom && py >= er.top) {
            if (!((CSBI_RectOnly*)m_guts)->FindReadySlot()) {
                return 1;
            }
            char ab = (char)g_644c54;
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            ((SbiHost*)m_4)->m_6c->Spawn(1, ab, 0, 0, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}
