// BootyMessages.cpp - the booty/secret-state HUD message overlays (graduated from
// src/Stub/Backlog.cpp). These are __thiscall methods on the booty game-state
// object (`this` carries the HUD message sink at +0xc, the secret gate at +0x1d4,
// the per-slot flag arrays at +0x284/+0x2a4, the level/secret gates at
// +0x200/+0x2f4). Each overlay builds one or more MFC CString temps (their dtors
// give the /GX exception frame) and hands each to the shared HUD message-sprite
// helper ShowHudMessage (0x1154b0, __cdecl(sink, &text, &rect, dur, ...)).
//
// IDENTITY recovered by string-xref (the "Level/World Completed!", "WARP letterz",
// "Secret Bonus" / "The Secret of Secretz:" message literals). The booty-state
// class, g_mgrSettings sub-objects and every callee are unmatched engine code
// reached by raw this+offset / reloc-masked external thunks; only the offsets /
// code bytes are load-bearing (campaign doctrine). Modeled with real <Mfc.h>
// CString so cl emits the same EH frame.

#include <Ints.h>
#include <Mfc.h> // CString temps (/GX) + RECT/CopyRect/SetRect

#include <rva.h>

// ---------------------------------------------------------------------------
// g_mgrSettings (0x64556c) - the game registry pointer. m_7c is the per-level
// record sub-object whose m_4 (= the level/area index), m_8 (a "suppress" gate),
// m_c (the world/training flag) and m_44 (a progress flag) gate which message is
// shown, plus the progress queries below. Only the touched offsets are modeled.
struct BzLevelRecord {
    char m_pad00[0x4];
    i32 m_4; // +0x04  area/level index
    i32 m_8; // +0x08  suppress gate
    i32 m_c; // +0x0c  world/training flag
    char m_pad10[0x44 - 0x10];
    i32 m_44; // +0x44  progress flag

    i32 GroupAllScored();      // 0xfce80 (thunk 0x2bda)
    i32 AllRecordsInBounds();  // 0xfccf0 (thunk 0x2bc6)
    float GroupRatio();        // 0xfce00 (thunk 0x30d0)
    i32 GetRecordValue();      // 0xfced0 (thunk 0x2bf3)
};
// The top-level window holder reached via g_mgrSettings->m_4; m_4 is its HWND.
struct BzWndHolder {
    char m_pad00[0x4];
    i32 m_4; // +0x04  HWND
};
// The looping-grunt idle sprite (m_1ec[]/m_2c8[]/m_2d8[] elements). CacheFirstFrame
// (0x150540) caches the named first frame; ApplyLookupGeometry (0x1505b0) resolves
// its cycle geometry. Both reloc-masked __thiscall.
struct BzSprite {
    void CacheFirstFrame(const char* name);             // 0x150540
    i32 ApplyLookupGeometry(const char* name, i32 def); // 0x1505b0
    char m_pad00[0x40];
    i32 m_40; // +0x40  visibility flags
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  sprite id
    i32 m_60; // +0x60  timer
};
// The ambient sound set reached via g_mgrSettings->m_30->m_28: m_10 is the named
// lookup table, m_30 the "is playing" gate.
struct BzSoundEntry {
    i32 Play(i32 tag, i32, i32, i32); // 0x1f940 (thunk 0x25fe)
};
struct BzFindTable {
    void Find(const char* name, BzSoundEntry** out); // 0x1b8438 (Lookup)
};
struct BzSoundSet {
    char m_pad00[0x10];
    BzFindTable m_10; // +0x10
    char m_pad14[0x30 - 0x14];
    i32 m_30; // +0x30  is-playing gate
};
struct BzSoundHolder {
    char m_pad00[0x28];
    BzSoundSet* m_28; // +0x28
};
struct BzGameReg {
    char m_pad00[0x4];
    BzWndHolder* m_4; // +0x04
    char m_pad08[0x30 - 0x8];
    BzSoundHolder* m_30; // +0x30
    char m_pad34[0x7c - 0x34];
    BzLevelRecord* m_7c; // +0x7c

    void ChangeState(i32); // 0x8fab0 (thunk 0x201d)
};
extern "C" BzGameReg* g_mgrSettings; // *0x64556c

// The HUD message sink (BzState::m_c). m_4 is the grunt-data loader (Load/Finish),
// m_8 a vtable-bearing notify object (slot +0x28 takes the loader's m_14), m_28 a
// dropped-sprite holder whose m_2c is freed on exit.
struct BzLoader {
    void Load();   // 0x158ee0
    void Finish(); // 0x158e90
    char m_pad00[0x14];
    void* m_14; // +0x14
};
struct BzSink8;
struct BzSink8Vtbl; // opaque; the notify slot lives at +0x28
typedef void (BzSink8::*BzNotifyFn)(void* arg);
struct BzSink8 {
    BzSink8Vtbl* m_vtbl; // +0x00
    void OnLoaded(void* arg);
};
struct BzSink8Vtbl {
    char m_pad00[0x28];
    BzNotifyFn OnLoaded; // +0x28
};
inline void BzSink8::OnLoaded(void* arg) { (this->*(m_vtbl->OnLoaded))(arg); }
struct BzSinkSub {
    void Free(); // 0x137a80
};
struct BzSinkSub28 {
    char m_pad00[0x2c];
    BzSinkSub* m_2c; // +0x2c
};
struct BzSink {
    char m_pad00[0x4];
    BzLoader* m_4; // +0x04
    BzSink8* m_8;  // +0x08
    char m_pad0c[0x28 - 0xc];
    BzSinkSub28* m_28; // +0x28
};

// Per-player idle-sprite id table (0x5e9068) + the trailing 4-sprite geometry
// table (0x5e8fe4, {timer, id} pairs), and the wand-cue ambient sound tag.
DATA(0x001e9068)
extern i32 g_idleSpriteIds[4]; // 0x5e9068
struct BzGeomPair {
    i32 m_60; // +0x00  timer
    i32 m_5c; // +0x04  sprite id
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
DATA(0x0022ae30)
extern char g_secretMsgA[0x20]; // 0x62ae30  encoded
DATA(0x0022ae50)
extern char g_secretMsgB[0x20]; // 0x62ae50  encoded
struct SecretMsgRow {
    char strA[0x20]; // +0x00  encoded line A
    char strB[0x80]; // +0x20  encoded line B
};
DATA(0x00229f30)
extern SecretMsgRow g_secretMsgRows[]; // 0x629f30  (0xa0 stride)
DATA(0x001e93b0)
extern float g_secretRatioScale; // 0x5e93b0

// The shared HUD message-sprite helper (0x1154b0, __cdecl): pushes a transient
// text sprite carrying `text` into `rect` with the given duration/colour flags.
void ShowHudMessage(void* sink, CString* text, RECT* rect, i32 dur, i32 a, i32 b,
                    i32 c, i32 d, i32 e); // 0x1154b0
// The status-line builder (0x1b2cf5, __cdecl(&dst, lpsz)).
void BzMsgFormat(CString* dst, const char* text); // 0x1b2cf5

// The booty/secret game-state object hosting the overlays.
class BzState {
public:
    void ShowLevelCompleteMessage();     // 0x1c9d0
    i32 ShowSecretBonusMessage();        // 0x18f00
    i32 BuildBootyGruntIdleAnimation();  // 0x1ce60
    // FormatHudText (0x1af70): fills `out` with the per-slot ready-message text.
    void FormatHudText(CString& out, i32 slot);                          // 0x1af70 (thunk 0x238d)
    i32 RegisterMultiNamespaces(const char* mode, i32, i32, i32, i32, i32); // 0xfa1f0 (thunk 0x1e60)
    void StartTimer(i32, i32, i32, i32);                                    // 0xfa8f0 (thunk 0x1843)
    void PassClickToPlayState(i32, i32, i32);                               // 0x8d780 (thunk 0x17c1)

    char m_pad00[0xc];
    BzSink* m_c; // +0x0c  HUD message sink
    char m_pad10[0x1b4 - 0x10];
    i32 m_1b4; // +0x1b4  armed flag
    char m_pad1b8[0x1bc - 0x1b8];
    i32 m_1bc; // +0x1bc  overlay state id (0xc7/0xc8/-2)
    char m_pad1c0[0x1d0 - 0x1c0];
    i32 m_1d0; // +0x1d0  init-once gate
    i32 m_1d4; // +0x1d4  secret-banner once gate
    char m_pad1d8[0x1ec - 0x1d8];
    BzSprite* m_1ec[4]; // +0x1ec  trailing idle sprites
    char m_pad1fc[0x200 - 0x1fc];
    i32 m_200; // +0x200  level-complete gate
    char m_pad204[0x284 - 0x204];
    i32 m_284[8]; // +0x284  per-slot "ready text" flags
    i32 m_2a4[8]; // +0x2a4  per-slot "template" flags
    char m_pad2c4[0x2c8 - 0x2c4];
    BzSprite* m_2c8[4]; // +0x2c8  per-player idle sprites (visibility)
    BzSprite* m_2d8[4]; // +0x2d8  per-player idle sprites (animation)
    char m_pad2e8[0x2f4 - 0x2e8];
    i32 m_2f4; // +0x2f4  secret-message gate
};

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
        if (m_2a4[i]) {
            RECT r1;
            CopyRect(&r1, &g_levelMsgRectsA[i]);
            CString t(g_levelMsgStrings[i]);
            ShowHudMessage(m_c, &t, &r1, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        if (m_284[i]) {
            RECT r2;
            CopyRect(&r2, &g_levelMsgRectsB[i]);
            CString t2;
            FormatHudText(t2, i);
            ShowHudMessage(m_c, &t2, &r2, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (m_200) {
        if (g_mgrSettings->m_7c->m_c != 0) {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("World Completed!");
            ShowHudMessage(m_c, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        } else {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("Level Completed!");
            ShowHudMessage(m_c, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        }
    }

    BzLevelRecord* rec = g_mgrSettings->m_7c;
    if (rec->m_8 == 0 && m_2f4 != 0) {
        CString s;
        RECT r;
        if (rec->m_4 > 0x24) {
            if (rec->m_c != 0) {
                s = "You have completed training! Now go and conquer the Battlez!";
            } else {
                s = "You are closer to achieving masterz status!";
            }
            SetRect(&r, 0x194, 0xaa, 0x263, 0x1e0);
        } else {
            if (rec->m_c != 0) {
                if (rec->GroupAllScored()) {
                    BzMsgFormat(&s, "WARP letterz recovered! Prepare to warp!");
                } else {
                    s = "WARP letterz not recovered! No checkpoint this time.";
                }
            } else {
                if (rec->m_44 != 0) {
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
// completed single record (m_1d4 + AllRecordsInBounds) it shows the static
// "Secret of Secretz" banner + the two cipher-decoded lines; otherwise it grades
// the group ratio into 1/2/3 rows and draws "Secret Bonus Acquired:" + that many
// cipher-decoded row pairs (each row offset by the level's rowBase).
// ===========================================================================
// @early-stop
// /GX EH-frame + nested-temp wall: complete + correct reconstruction (the m_1d4/
// AllRecordsInBounds banner arm, the GroupRatio*scale -> 1/2/3 grading, the
// rowBase=(m_4-1)/4 row index, the per-(category,row) SetRect coordinate table, and
// the `-=0x3d` SetAt decode cipher). Residual = the delinked `Unwind@..` EH frame
// (docs/seh-eh.md) + the per-CString-temp EH-state ordering / callee-saved regalloc
// across the many destructible RECT+CString locals. Logic + externs/strings named.
RVA(0x00018f00, 0x4fb)
i32 BzState::ShowSecretBonusMessage() {
    if (m_1d4 != 0 && g_mgrSettings->m_7c->AllRecordsInBounds()) {
        CString s;
        if (!RegisterMultiNamespaces("multi", 0, 0, 0, 0, 1)) {
            return 0;
        }
        RECT r1, r2, r3;
        SetRect(&r1, 0, -15, 0x280, 0x1d1);
        SetRect(&r2, 0, 0x19, 0x280, 0x1f9);
        SetRect(&r3, 0, 0x38, 0x280, 0x78);
        BzMsgFormat(&s, "The Secret of Secretz:");
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

    i32 count = (i32)(g_mgrSettings->m_7c->GroupRatio() * g_secretRatioScale);
    i32 rowBase = (g_mgrSettings->m_7c->m_4 - 1) / 4;
    i32 category = (count >= 0x64) ? 3 : ((count >= 0x32) ? 2 : 1);

    if (!RegisterMultiNamespaces("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }
    CString title;
    RECT rTitle;
    SetRect(&rTitle, 0, 0x38, 0x280, 0x78);
    BzMsgFormat(&title, "Secret Bonus Acquired:");
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
// idiom) reusing the BzState helpers above.
// ===========================================================================
// @early-stop
// /GX EH-frame + sub-object-regalloc wall: complete + correct reconstruction (the
// 0xc7/0xc8 guard, the m_8 PostMessage arm, the m_1d0 init path with the wand-cue
// sound, the 4-player loop + WARP jump-table CString build, the trailing-sprite
// geometry loop, and the m_1d0!=0 step/tick arms grading the secret bonus). Residual
// = the delinked `Unwind@..` EH frame (docs/seh-eh.md) + callee-saved regalloc across
// the many engine sub-objects reached by raw this+offset. Logic + externs/strings named.
RVA(0x0001ce60, 0x450)
i32 BzState::BuildBootyGruntIdleAnimation() {
    i32 state = m_1bc;
    if (state != 0xc7 && state != 0xc8) {
        m_1b4 = 1;
        return 1;
    }
    BzLevelRecord* rec = g_mgrSettings->m_7c;
    if (rec->m_8 != 0) {
        PostMessageA((HWND)g_mgrSettings->m_4->m_4, 0x111, 0x8023, 0);
        return 1;
    }
    if (m_1d0 == 0) {
        if (rec->m_c != 0) {
            m_1d0 = 1;
            BzSoundSet* ss = g_mgrSettings->m_30->m_28;
            if (ss->m_30 == 0) {
                BzSoundEntry* res = 0;
                ss->m_10.Find("GRUNTZ_WANDGRUNT_WANDZGRUNTI3A", &res);
                if (res != 0) {
                    res->Play(g_sndCueTag, 0, 0, 0);
                }
            }
            if (g_mgrSettings->m_7c->m_4 < 0x24) {
                for (i32 p = 0; p < 4; p++) {
                    m_2c8[p]->m_40 |= 1;
                    m_2d8[p]->m_5c = g_idleSpriteIds[p];
                    m_2d8[p]->m_60 = 0xdc;
                    m_2d8[p]->m_40 &= ~1;
                    if (g_mgrSettings->m_7c->GetRecordValue() != 0) {
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
                        m_2d8[p]->CacheFirstFrame("GRUNTZ_PICKUPS");
                        m_2d8[p]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    } else {
                        m_2d8[p]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_2d8[p]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    }
                }
            }
            for (i32 k = 0; k < 4; k++) {
                m_1ec[k]->m_5c = g_idleGeom[k].m_5c;
                m_1ec[k]->m_60 = g_idleGeom[k].m_60;
                m_1ec[k]->m_40 &= ~1;
            }
            if (!RegisterMultiNamespaces("bg", 0, 0, 0, 0, 1)) {
                return 0;
            }
            ShowLevelCompleteMessage();
            m_c->m_4->Load();
            m_c->m_8->OnLoaded(m_c->m_4->m_14);
            m_c->m_4->Finish();
            StartTimer(0x50, 0x3e8, 0, 1);
            if (!RegisterMultiNamespaces("bg", 0, 0, 0, 0, 1)) {
                return 0;
            }
            ShowLevelCompleteMessage();
            return 1;
        }
    } else if (rec->m_c != 0 && rec->m_4 < 0x24 && state == 0xc7) {
        if (rec->GroupAllScored()) {
            if (!ShowSecretBonusMessage()) {
                return 0;
            }
            m_c->m_4->Load();
            StartTimer(0x50, 0x3e8, 0, 1);
            m_1bc = 0xfffffffe;
            return 1;
        }
    }

    if (m_1bc == 0xfffffffe && g_mgrSettings->m_7c->AllRecordsInBounds() && m_1d4 == 0) {
        m_1d4 = 1;
        if (!ShowSecretBonusMessage()) {
            return 0;
        }
        m_c->m_4->Load();
        StartTimer(0x50, 0x3e8, 0, 1);
        return 1;
    }

    BzLevelRecord* rec2 = g_mgrSettings->m_7c;
    if (rec2->m_4 == 0x20) {
        BzSinkSub* sub = m_c->m_28->m_2c;
        if (sub != 0) {
            sub->Free();
        }
        g_mgrSettings->ChangeState(3);
        PostMessageA((HWND)g_mgrSettings->m_4->m_4, 0x111, 0x8021, 0);
    } else {
        PassClickToPlayState((rec2->m_4 % 0x28) + 1, 0, 1);
    }
    return 1;
}
