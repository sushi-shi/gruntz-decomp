// BootyMessages.cpp - the booty/secret-state HUD message overlays. These are
// __thiscall methods on the booty game-state
// object (`this` carries the HUD message sink at +0xc, the secret gate at +0x1d4,
// the per-slot flag arrays at +0x284/+0x2a4, the level/secret gates at
// +0x200/+0x2f4). Each overlay builds one or more MFC CString temps (their dtors
// give the /GX exception frame) and hands each to the shared HUD message-sprite
// helper ShowHudMessage (0x1154b0, __cdecl(sink, &text, &rect, dur, ...)).
//
// IDENTITY recovered by string-xref (the "Level/World Completed!", "WARP letterz",
// "Secret Bonus" / "The Secret of Secretz:" message literals). The booty-state
// class, g_mgrSettings sub-objects and every callee are unmatched engine code
// reached by reloc-masked external thunks; only the offsets / code bytes are
// load-bearing (campaign doctrine). The Bz* object graph is the shared
// <Gruntz/BzState.h>; modeled with real <Mfc.h> CString so cl emits the same EH
// frame.

#include <Ints.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/LeafCue.h>
#include <Mfc.h> // CString temps (/GX) + RECT/CopyRect/SetRect

#include <Gruntz/BzState.h>

#include <rva.h>
#include <Globals.h>

// Per-player idle-sprite id table (0x5e9068) + the trailing 4-sprite geometry
// table (0x5e8fe4, {y, x} pairs - onscreen coordinates), and the wand-cue ambient sound tag.
struct BzGeomPair {
    i32 m_y; // +0x00  onscreen y
    i32 m_x; // +0x04  onscreen x
};
DATA(0x001e8fe4)
extern BzGeomPair g_idleGeom[4]; // 0x5e8fe4
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24 (?g_sndCueTag@@3HA)

// The 8 level-complete message templates (global CString array at 0x629ef8) and
// the two parallel RECT arrays they are drawn into.
DATA(0x00229ef8)
extern CString g_levelMsgStrings[8]; // 0x629ef8
DATA(0x0020b838)
extern RECT g_levelMsgRectsA[8]; // 0x60b838
DATA(0x0020b8f8)
extern RECT g_levelMsgRectsB[8]; // 0x60b8f8

// The secret-bonus message tables: a "+0x3d"-encoded buffer pair (decoded in place
// by the SetAt cipher) for the single-record banner, plus the per-row table indexed
// by (rowBase*3 + row), each row a 0xa0-byte record carrying two encoded strings.
struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
DATA(0x00229f30)
extern SecretMsgRow g_secretMsgRows[]; // 0x629f30  (0xa0 stride)

// The shared HUD message-sprite helper (0x1154b0, __cdecl): pushes a transient
// text sprite carrying `text` into `rect` with the given duration/colour flags.
void ShowHudMessage(
    void* sink,
    CString* text,
    RECT* rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0

// ===========================================================================
// ShowLevelCompleteMessage @0x1c9d0 - draws the per-slot ready/template overlays,
// then the level/world-completed banner, then the WARP-letterz status line.
// ===========================================================================
// @early-stop
// /GX EH-frame wall (~98%): complete + correct, verified instruction-by-instruction.
// Residual = the delinked target's `Unwind@..`/no-`__except_list` EH representation
// vs cl's `$L..`+`__except_list` frame (docs/seh-eh.md), plus a callee-saved-reg
// NAMING choice in the post-loop banner/WARP blocks (cl hoists the 0x24 rect-top
// constant into ebx, retail into edi; equivalent). Logic + all externs/strings named.
RVA(0x0001c9d0, 0x351)
void BzState::ShowLevelCompleteMessage() {
    for (i32 i = 0; i < 8; i++) {
        if (m_templateFlags[i]) {
            RECT r1;
            CopyRect(&r1, &g_levelMsgRectsA[i]);
            CString t(g_levelMsgStrings[i]);
            ShowHudMessage(m_sink, &t, &r1, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        if (m_readyFlags[i]) {
            RECT r2;
            CopyRect(&r2, &g_levelMsgRectsB[i]);
            CString t2;
            FormatHudText(t2, i);
            ShowHudMessage(m_sink, &t2, &r2, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (m_levelCompleteGate) {
        if (g_mgrSettings->m_levelRecord->m_worldFlag != 0) {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("World Completed!");
            ShowHudMessage(m_sink, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        } else {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("Level Completed!");
            ShowHudMessage(m_sink, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        }
    }

    BzLevelRecord* rec = g_mgrSettings->m_levelRecord;
    if (rec->m_suppressGate == 0 && m_secretGate != 0) {
        CString s;
        RECT r;
        if (rec->m_levelIndex > 0x24) {
            if (rec->m_worldFlag != 0) {
                s = "You have completed training! Now go and conquer the Battlez!";
            } else {
                s = "You are closer to achieving masterz status!";
            }
            SetRect(&r, 0x194, 0xaa, 0x263, 0x1e0);
        } else {
            if (rec->m_worldFlag != 0) {
                if (((CBattlezData*)rec)->GroupAllScored()) {
                    s.Format("WARP letterz recovered! Prepare to warp!");
                } else {
                    s = "WARP letterz not recovered! No checkpoint this time.";
                }
            } else {
                if (rec->m_progressFlag != 0) {
                    s = "Keep finding those WARP letterz!";
                } else {
                    s = "Collect all four WARP letterz to reach the checkpoint!";
                }
            }
            SetRect(&r, 0x194, 0xe6, 0x263, 0x1e0);
        }
        ShowHudMessage(m_sink, &s, &r, 0x6e, 1, 0xff, 0xff, 0, 1);
    }
}

SIZE_UNKNOWN(BzGeomPair);
SIZE_UNKNOWN(SecretMsgRow);
