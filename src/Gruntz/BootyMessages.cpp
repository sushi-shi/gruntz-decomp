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
// class, g_gameReg sub-objects and every callee are unmatched engine code
// reached by reloc-masked external thunks; only the offsets / code bytes are
// load-bearing (campaign doctrine). The Bz* object graph is the shared
// <Gruntz/BzState.h>; modeled with real <Mfc.h> CString so cl emits the same EH
// frame.

#include <Ints.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/LeafCue.h> // LeafCue + SoundStream (the CSndHost+0x2c stream) via SoundCue.h
#include <Mfc.h>            // CString temps (/GX) + RECT/CopyRect/SetRect

#include <Gruntz/GameMode.h> // canonical CBootyState : CState (the folded booty state)
#include <Gruntz/BzState.h>  // the deferred g_gameReg sub-object views (BzSink is dissolved)

#include <rva.h>
#include <Globals.h>

// Per-player idle-sprite id table (0x5e9068) + the trailing 4-sprite geometry
// table (0x5e8fe4, {y, x} pairs - onscreen coordinates), and the wand-cue ambient sound tag.
struct BzGeomPair {
    i32 m_y; // +0x00  onscreen y
    i32 m_x; // +0x04  onscreen x
};
// Owner-TU definition, TRANSCRIBED from the retail .rdata bytes @0x5e8fe4
// ({0,472} {101,525} {98,474} {146,525} - the four idle-sprite onscreen spots).
DATA(0x001e8fe4)
BzGeomPair g_idleGeom[4] = {
    {0, 472},
    {101, 525},
    {98, 474},
    {146, 525},
};
// The secret-bonus group-ratio scale (0x5e93b0, .rdata const 100.0); DEFINED here
// (owner TU), reference extern stays in <Globals.h>. (REHOME DD-G)
DATA(0x001e93b0)
float g_secretRatioScale = 100.0f; // 0x5e93b0
extern i32 g_sndCueTag;            // 0x61ab24 (?g_sndCueTag@@3HA)

// The 8 level-complete message templates (global CString array at 0x629ef8, .bss;
// the CRT default-constructs the elements at startup) and the two parallel RECT
// arrays they are drawn into. Owner-TU definition.
DATA(0x00229ef8)
CString g_levelMsgStrings[8]; // 0x629ef8
DATA(0x0020b838)
RECT g_levelMsgRectsA[8] = {
    {105, 106, 190, 155},
    {26, 149, 182, 199},
    {72, 192, 187, 240},
    {87, 238, 185, 288},
    {94, 281, 185, 332},
    {31, 324, 182, 374},
    {89, 360, 181, 411},
    {59, 400, 180, 449}
};
DATA(0x0020b8f8)
RECT g_levelMsgRectsB[8] = {
    {245, 92, 417, 162},
    {245, 135, 417, 205},
    {245, 180, 417, 250},
    {245, 227, 417, 297},
    {245, 266, 417, 340},
    {245, 310, 417, 380},
    {245, 351, 417, 421},
    {245, 392, 417, 462}
};

// The secret-bonus message tables: a "+0x3d"-encoded buffer pair (decoded in place
// by the SetAt cipher) for the single-record banner, plus the per-row table indexed
// by (rowBase*3 + row), each row a 0xa0-byte record carrying two encoded strings.
struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
// Owner-TU definition (.bss, runtime-filled). Row count CODE+DOMAIN-PROVEN, not
// gap-guessed: idx = rowBase*3 + j with j in 0..2 and rowBase = (levelIndex-1)/4,
// levelIndex <= 32 (8 worldz x 4 levelz) -> rowBase <= 7 -> 24 rows.
DATA(0x00229f30)
SecretMsgRow g_secretMsgRows[24]; // 0x629f30  (0xa0 stride)
// The single-record banner's encoded string pair (.bss, decoded in place at runtime),
// laid DIRECTLY AFTER the row table (0x629f30+0xf00 = 0x62ae30/0x62ae50); DEFINED
// here (owner TU), reference externs stay in <Globals.h>. (DD-G)
char g_secretMsgA[0x20]; // 0x62ae30  encoded line A
char g_secretMsgB[0x80]; // 0x62ae50  encoded line B (strB extent 0x80, not 0x20)

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

// The booty HUD message sink IS the inherited CState::m_c holder (the canonical
// CSpriteFactoryHolder): m_pages (+0x04) the page sub-manager, m_8 (+0x08) the
// object-group render broadcaster, m_28 (+0x28) the cue host whose m_2c is the
// held SoundStream. (The former BzSink view + its BzSink8 placeholder vtable are
// dissolved - see <Gruntz/BzState.h>.)

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
// reloc-fidelity: FOLDED - now the real CBootyState:: method (was a BzState view + a
// SYMBOL override). m_sink@+0xc == inherited CState::m_c; the CState-slot-8 activator
// (StateImages::InputVirtual) binds to this canonical symbol structurally.
RVA(0x0001c9d0, 0x351)
void CBootyState::ShowLevelCompleteMessage() {
    for (i32 i = 0; i < 8; i++) {
        if (m_templateFlags[i]) {
            RECT r1;
            CopyRect(&r1, &g_levelMsgRectsA[i]);
            CString t(g_levelMsgStrings[i]);
            ShowHudMessage(m_c, &t, &r1, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        if (m_readyFlags[i]) {
            RECT r2;
            CopyRect(&r2, &g_levelMsgRectsB[i]);
            CString t2;
            FormatHudText(&t2, i);
            ShowHudMessage(m_c, &t2, &r2, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (m_levelCompleteGate) {
        if (g_gameReg->m_levelRecord->m_worldFlag != 0) {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("World Completed!");
            ShowHudMessage(m_c, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        } else {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("Level Completed!");
            ShowHudMessage(m_c, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        }
    }

    BzLevelRecord* rec = g_gameReg->m_levelRecord;
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
        ShowHudMessage(m_c, &s, &r, 0x6e, 1, 0xff, 0xff, 0, 1);
    }
}

// ===========================================================================
// ShowSecretBonusMessage @0x18f00 - draws the "Secret Bonus" overlay. With a
// completed single record (m_secretBannerOnce + AllRecordsInBounds) it shows the
// static "Secret of Secretz" banner + the two cipher-decoded lines; otherwise it
// grades the group ratio into 1/2/3 rows and draws "Secret Bonus Acquired:" + that
// many cipher-decoded row pairs (each row offset by the level's rowBase).
// ===========================================================================
// @early-stop
// /GX EH-frame + nested-temp wall: complete + correct reconstruction (the
// m_secretBannerOnce/AllRecordsInBounds banner arm, the GroupRatio*scale -> 1/2/3
// grading, the rowBase=(m_levelIndex-1)/4 row index, the per-(category,row) SetRect
// coordinate table, and the `-=0x3d` SetAt decode cipher). Residual = the delinked
// `Unwind@..` EH frame (docs/seh-eh.md) + the per-CString-temp EH-state ordering /
// callee-saved regalloc across the many destructible RECT+CString locals. Logic +
// externs/strings named.
// reloc-fidelity: FOLDED - now the real CBootyState:: method (was a BzState view + a
// SYMBOL override). RegisterMultiNamespaces == inherited CState::FadeInTitle (0xfa1f0).
// StateImages::InputVirtual binds to this canonical symbol structurally.
RVA(0x00018f00, 0x4fb)
i32 CBootyState::ShowSecretBonusMessage() {
    if (m_secretBannerOnce != 0
        && ((CBattlezData*)g_gameReg->m_levelRecord)->AllRecordsInBounds()) {
        CString s;
        if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
            return 0;
        }
        RECT r1, r2, r3;
        SetRect(&r1, 0, -15, 0x280, 0x1d1);
        SetRect(&r2, 0, 0x19, 0x280, 0x1f9);
        SetRect(&r3, 0, 0x38, 0x280, 0x78);
        s.Format("The Secret of Secretz:");
        ShowHudMessage(m_c, &s, &r1, 0x82, 1, 0xff, 0xff, 0, 1);

        CString s2(g_secretMsgA);
        CString s3(g_secretMsgB);
        for (i32 k = 0; k < s2.GetLength(); k++) {
            s2.SetAt(k, (char)(((const char*)s2)[k] - 0x3d));
        }
        ShowHudMessage(m_c, &s2, &r3, 0x78, 1, 0xff, 0xff, 0, 1);
        ShowHudMessage(m_c, &s3, &r2, 0x6e, 1, 0xff, 0xff, 0, 1);
        return 1;
    }

    i32 count = (i32)(((CBattlezData*)g_gameReg->m_levelRecord)->GroupRatio() * g_secretRatioScale);
    i32 rowBase = (g_gameReg->m_levelRecord->m_levelIndex - 1) / 4;
    i32 category = (count >= 0x64) ? 3 : ((count >= 0x32) ? 2 : 1);

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }
    CString title;
    RECT rTitle;
    SetRect(&rTitle, 0, 0x38, 0x280, 0x78);
    title.Format("Secret Bonus Acquired:");
    ShowHudMessage(m_c, &title, &rTitle, 0x82, 1, 0xff, 0xff, 0, 1);

    for (i32 j = 0; j < category; j++) {
        RECT rA, rB;
        if (category == 1) {
            SetRect(&rA, 0, -15, 0x280, 0x1d1);
            SetRect(&rB, 0, 0x19, 0x280, 0x1f9);
        } else if (category == 2) {
            if (j == 0) {
                SetRect(&rA, 0, -20, 0x280, 0x1cc);
                SetRect(&rB, 0, 0x14, 0x280, 0x1f4);
            } else {
                SetRect(&rA, 0, 0x46, 0x280, 0x226);
                SetRect(&rB, 0, 0x6e, 0x280, 0x24e);
            }
        } else {
            if (j == 0) {
                SetRect(&rA, 0, -60, 0x280, 0x1a4);
                SetRect(&rB, 0, -20, 0x280, 0x1cc);
            } else if (j == 1) {
                SetRect(&rA, 0, 0x1e, 0x280, 0x1fe);
                SetRect(&rB, 0, 0x46, 0x280, 0x226);
            } else {
                SetRect(&rA, 0, 0x78, 0x280, 0x24e);
                SetRect(&rB, 0, 0xa0, 0x280, 0x276);
            }
        }
        i32 idx = rowBase * 3 + j;
        CString s5(g_secretMsgRows[idx].strA);
        CString s6(g_secretMsgRows[idx].strB);
        for (i32 k = 0; k < s5.GetLength(); k++) {
            s5.SetAt(k, (char)(((const char*)s5)[k] - 0x3d));
        }
        ShowHudMessage(m_c, &s5, &rA, 0x78, 1, 0xff, 0xff, 0, 1);
        ShowHudMessage(m_c, &s6, &rB, 0x6e, 1, 0xff, 0xff, 0, 1);
    }
    return 1;
}

// ===========================================================================
// BuildBootyGruntIdleAnimation @0x1ce60 - the per-frame booty/secret idle-grunt
// animation state machine. On the first entry in state 0xc7/0xc8 it installs the
// "bg" namespace, seeds the four player idle grunts (visibility + the per-player
// "GRUNTZ_PICKUPS_<W/A/R/P>" or "GRUNTZ_NORMALGRUNT" cycle) and the trailing idle
// sprites, then kicks the loader + timer; on later ticks it grades the secret bonus
// and advances/ends the state. Sibling of CGruntSprintAnim (same CString cycle-name
// idiom) reusing the shared CBootyState helpers above.
// ===========================================================================
// @early-stop
// /GX EH-frame + sub-object-regalloc wall: complete + correct reconstruction (the
// 0xc7/0xc8 guard, the m_suppressGate PostMessage arm, the m_initOnce init path with
// the wand-cue sound, the 4-player loop + WARP jump-table CString build, the
// trailing-sprite geometry loop, and the m_initOnce!=0 step/tick arms grading the
// secret bonus). Residual = the delinked `Unwind@..` EH frame (docs/seh-eh.md) +
// callee-saved regalloc across the many engine sub-objects reached by raw
// this+offset. Logic + externs/strings named.
// reloc-fidelity: FOLDED - now the real CBootyState:: method (was a BzState view + a
// SYMBOL override), so BootyStateActivate's slot 12/14/17 tail-callers bind structurally.
// RegisterMultiNamespaces == CState::FadeInTitle (0xfa1f0), StartTimer == BuildPage
// (0xfa8f0), PassClickToPlayState == CGruntzMgr::PassClickToPlayState (0x8d780, ecx=reg).
RVA(0x0001ce60, 0x450)
i32 CBootyState::BuildBootyGruntIdleAnimation() {
    i32 state = m_activation;
    if (state != 0xc7 && state != 0xc8) {
        m_initGate = 1;
        return 1;
    }
    BzLevelRecord* rec = g_gameReg->m_levelRecord;
    if (rec->m_suppressGate != 0) {
        PostMessageA((HWND)g_gameReg->m_wnd->m_hwnd, 0x111, 0x8023, 0);
        return 1;
    }
    if (m_initOnce == 0) {
        if (rec->m_worldFlag != 0) {
            m_initOnce = 1;
            BzSoundSet* ss = g_gameReg->m_soundHolder->m_soundSet;
            if (ss->m_playing == 0) {
                BzSoundEntry* res = 0;
                ((CMapStringToPtr*)&ss->m_findTable)
                    ->Lookup("GRUNTZ_WANDGRUNT_WANDZGRUNTI3A", (void*&)res);
                if (res != 0) {
                    ((LeafCue*)res)->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);
                }
            }
            if (g_gameReg->m_levelRecord->m_levelIndex < 0x24) {
                for (i32 p = 0; p < 4; p++) {
                    m_visSprites[p]->m_stateFlags |= 1;
                    m_animSprites[p]->m_screenX = g_idleSpriteIds[p];
                    m_animSprites[p]->m_screenY = 0xdc;
                    m_animSprites[p]->m_stateFlags &= ~1;
                    if (((CBattlezData*)g_gameReg->m_levelRecord)->GetRecordValue(p) != 0) {
                        CString letter;
                        switch (p) {
                            case 0:
                                letter = "W";
                                break;
                            case 1:
                                letter = "A";
                                break;
                            case 2:
                                letter = "R";
                                break;
                            case 3:
                                letter = "P";
                                break;
                        }
                        m_animSprites[p]->ApplyName("GRUNTZ_PICKUPS");
                        m_animSprites[p]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    } else {
                        m_animSprites[p]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[p]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    }
                }
            }
            for (i32 k = 0; k < 4; k++) {
                m_trailSprites[k]->m_screenX = g_idleGeom[k].m_x;
                m_trailSprites[k]->m_screenY = g_idleGeom[k].m_y;
                m_trailSprites[k]->m_stateFlags &= ~1;
            }
            if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
                return 0;
            }
            ShowLevelCompleteMessage();
            m_c->m_pages->Method_158ee0();
            m_c->m_8->WalkDispatch2C(m_c->m_pages->m_backPair);
            m_c->m_pages->Method_158e90();
            BuildPage(0x50, 0x3e8, 0, 1);
            if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
                return 0;
            }
            ShowLevelCompleteMessage();
            return 1;
        }
    } else if (rec->m_worldFlag != 0 && rec->m_levelIndex < 0x24 && state == 0xc7) {
        if (((CBattlezData*)rec)->GroupAllScored()) {
            if (!ShowSecretBonusMessage()) {
                return 0;
            }
            m_c->m_pages->Method_158ee0();
            BuildPage(0x50, 0x3e8, 0, 1);
            m_activation = 0xfffffffe;
            return 1;
        }
    }

    if (m_activation == 0xfffffffe
        && ((CBattlezData*)g_gameReg->m_levelRecord)->AllRecordsInBounds()
        && m_secretBannerOnce == 0) {
        m_secretBannerOnce = 1;
        if (!ShowSecretBonusMessage()) {
            return 0;
        }
        m_c->m_pages->Method_158ee0();
        BuildPage(0x50, 0x3e8, 0, 1);
        return 1;
    }

    BzLevelRecord* rec2 = g_gameReg->m_levelRecord;
    if (rec2->m_levelIndex == 0x20) {
        SoundStream* sub = m_c->m_28->m_2c;
        if (sub != 0) {
            sub->Stop();
        }
        g_gameReg->ChangeState_8fab0(3);
        PostMessageA((HWND)g_gameReg->m_wnd->m_hwnd, 0x111, 0x8021, 0);
    } else {
        // 0x8d780: DISASM-PROVEN receiver ecx = *0x24556c (the CGruntzMgr singleton),
        // NOT `this`; g_gameReg IS that singleton, so cast the view to its real class.
        ((CGruntzMgr*)g_gameReg)->PassClickToPlayState((rec2->m_levelIndex % 0x28) + 1, 0, 1);
    }
    return 1;
}

SIZE_UNKNOWN(BzGeomPair);
SIZE_UNKNOWN(SecretMsgRow);
