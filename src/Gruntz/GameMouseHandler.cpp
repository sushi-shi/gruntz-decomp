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
#include <Win32.h> // SetRect / RECT / POINT

#include <rva.h>

SIZE_UNKNOWN(SbiSndEntry);
struct SbiSndEntry { // a found sound entry; PlayCue is __thiscall on the entry
    void PlayCue(i32 token, i32, i32, i32); // FUN @ 0x25fe (thunk) __thiscall
};
SIZE_UNKNOWN(SbiSndTable);
struct SbiSndTable {
    void Find(char* szName, SbiSndEntry** out); // FUN_001b8438 __thiscall, out-param
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
SIZE_UNKNOWN(SbiProbe);
struct SbiProbe { // m_4->m_68
    // FUN @ 0x3cb0 __thiscall: probe the area map under (x,y).
    i32 Probe(i32 x, i32 y, i32* outArea, i32* outVal, i32 flag);
};
SIZE_UNKNOWN(SbiSpawner);
struct SbiSpawner { // m_4->m_6c
    // FUN @ 0x2095 __thiscall: spawn the tab eye-candy at a tile origin.
    void Spawn(i32 a1, char area, i32 a3, i32 a4, i32 px, i32 py, i32 a7, i32 a8);
};
SIZE_UNKNOWN(SbiHost);
struct SbiHost { // this->m_4
    char m_pad00[0x30];
    SbiHostInner* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    SbiProbe* m_68;   // +0x68
    SbiSpawner* m_6c; // +0x6c
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
struct SbiChild {                        // this->m_2dc
    i32 m_0;                             // +0x00  mode tag
    i32 HitTest(i32 x, i32 y);           // FUN @ 0x3ad5 __thiscall, ret index/-1
    void Select(i32 idx, i32 flag);      // FUN @ 0x20b8 __thiscall
    void Refresh();                      // FUN @ 0x123f __thiscall
    void Notify(i32 v);                  // FUN @ 0x4179 __thiscall
    i32 Check();                         // FUN @ 0x3549 __thiscall, ret bool
    i32 Dispatch(i32 msg, i32 x, i32 y); // FUN @ 0x1b81 __thiscall, ret result
};
SIZE_UNKNOWN(SbiToggle);
struct SbiToggle {   // this->m_2e0
    void Set(i32 n); // FUN @ 0x171c __thiscall
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
struct SbiVtbl; // completed below (carries the base-handler PMF at slot +0x38)

SIZE_UNKNOWN(CPlay);
class CPlay {
public:
    i32 vfunc_16(i32 msg, i32 x, i32 y); // slot 16 (+0x40) @0xce660

    SbiVtbl* m_vptr; // +0x000
    SbiHost* m_4;    // +0x004
    char m_pad08[0xc - 0x8];
    SbiRectHost* m_c; // +0x00c
    char m_pad10[0x2dc - 0x10];
    SbiChild* m_2dc;  // +0x2dc
    SbiToggle* m_2e0; // +0x2e0
    char m_pad2e4[0x368 - 0x2e4];
    i32 m_368; // +0x368
    i32 m_36c; // +0x36c
    char m_pad370[0x374 - 0x370];
    SbiEntry** m_374; // +0x374
    i32 m_378;        // +0x378
    char m_pad37c[0x484 - 0x37c];
    i32 m_484; // +0x484
    char m_pad488[0x4fc - 0x488];
    i32 m_4fc; // +0x4fc
};
// The base-class handler at vtable slot +0x38, modeled as a 4-byte PMF off the
// vtable so MSVC emits `mov edx,[this]; mov ecx,this; call [edx+0x38]`. The class
// must be COMPLETE before this typedef - see docs/patterns/pmf-complete-class-4byte.md.
typedef i32 (CPlay::*SbiSlot38Fn)(i32, i32, i32);
SIZE_UNKNOWN(SbiVtbl);
struct SbiVtbl {
    char m_pad00[0x38];
    SbiSlot38Fn slot38; // +0x38
};

// @early-stop
// regalloc/frame-slot wall (~52%): logic complete + verified instruction-by-
// instruction vs retail (every guard, the Dispatch/vtable delegates, the mode-2
// cue arm, HitTest/Select, the rect test, Probe, and the tile-align spawn loop all
// match), and dropping the `m_2dc` local cache already pinned `this` to edi exactly
// like retail. The sole residual: retail's 0x10 frame reuses the dead x-arg home for
// the Probe out-param, while this /O2 build gives it a fresh 5th slot ([esp+0x10]) ->
// a 0x14 frame that +4-shifts every [esp+N] operand. Not source-steerable. See
// docs/patterns/zero-register-pinning.md + identical-return-epilogue-tailmerge.md.
RVA(0x000ce660, 0x362)
i32 CPlay::vfunc_16(i32 msg, i32 x, i32 y) {
    if (m_484 != 0) {
        return 1;
    }
    if (m_2dc == 0) {
        return 1;
    }
    if (m_4fc != 0) {
        return m_2dc->Dispatch(msg, x, y);
    }
    if (g_sbiMgr->m_68->m_400 == 0) {
        return m_2dc->Dispatch(msg, x, y);
    }
    if (m_368 != 0 || m_36c != 0) {
        return (this->*(m_vptr->slot38))(msg, x, y);
    }

    if (m_2dc->m_0 == 2 && SbiPointInChild(x, y)) {
        SbiSndSet* set = m_4->m_30->m_28;
        if (set->m_30 == 0) {
            SbiSndEntry* e = 0;
            set->m_10.Find("GAME_TABHIGHLIGHT1", &e);
            if (e != 0) {
                e->PlayCue(g_sndCueTag, 0, 0, 0);
            }
        }
        m_2dc->Refresh();
        if (m_2dc->m_0 == 1) {
            m_2e0->Set(2);
        } else {
            m_2e0->Set(1);
        }
        return 1;
    }

    i32 idx = m_2dc->HitTest(x, y);
    if (idx != -1) {
        m_2dc->Select(idx, 1);
        return 1;
    }

    RECT* rc = &m_c->m_24->m_10;
    i32 rl = rc->left;
    i32 rt = rc->top;
    i32 rr = rc->right;
    i32 rb = rc->bottom;
    if (x < rl || x > rr || y < rt || y > rb) {
        return m_2dc->Dispatch(msg, x, y);
    }

    i32 outArea;
    i32 outVal;
    if (m_4->m_68->Probe(x, y, &outArea, &outVal, 5) && g_644c54 == outArea) {
        m_2dc->Notify(outVal);
        return 1;
    }

    if (m_368 != 0) {
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

    SbiHost24* h = m_4->m_30->m_24;
    i32 px = h->m_5c->m_40.x - h->m_10 + x;
    i32 py = h->m_5c->m_40.y - h->m_14 + y;
    for (i32 i = 0; i < m_378; i++) {
        SbiEntry* e = m_374[i];
        if (e == 0) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_0 - 0x10, e->m_4 - 0x10, e->m_0 + 0x10, e->m_4 + 0x10);
        if (px < er.right && px >= er.left && py < er.bottom && py >= er.top) {
            if (!m_2dc->Check()) {
                return 1;
            }
            char ab = (char)g_644c54;
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_4->m_6c->Spawn(1, ab, 0, 0, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}
