// m5_LevelPreview.cpp - the level-preview screen tick (RVA 0xde420).
//
// Advances the PREVIEW%i counter, resolves the next \SCREENZ\%s namespace, fades
// its title in and (when the live surface is free) plays the teleporter-open cue;
// on a failed fade it cancels the command. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#include <rva.h>

#include <stdio.h>

#include <Bute/SymTab.h>

extern "C" {
    DATA(0x0061ab20)
    extern i32 g_sndEnabled;
    DATA(0x0061ab24)
    extern i32 g_sndCueTag;
    DATA(0x006bf3c0)
    extern u32 g_killCueClock;
}

struct CString {
    char* m_pchData; // +0x00
    CString& operator=(const char* s); // 0x1b9e74
};

class CStatusBarMgr {
public:
    i32 ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1360d0
};
struct CueObj {
    char m_pad00[0x10];
    CStatusBarMgr* m_10; // +0x10
    i32 m_14;            // +0x14
    i32 m_18;            // +0x18
};
class CSpriteHashTable {
public:
    i32 Lookup(const char* szName, CueObj** ppOut); // 0x1b8438
};
struct CStatusBarHolder {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
    char m_pad14[0x30 - 0x14];
    i32 m_30; // +0x30
};
struct PreviewMgr {
    char m_pad00[0x28];
    CStatusBarHolder* m_28; // +0x28
};

class CPreviewState {
public:
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0
    void RetireScene(i32 a, i32 b, i32 c, i32 d);                  // 0x0fa8f0
    void Cancel();                                                 // 0x0de590
    void LoadLevelPreviewScreen();

    char m_pad000[0x0c];
    PreviewMgr* m_0c; // +0x0c
    char m_pad10[0x2c - 0x10];
    CSymTab* m_2c; // +0x2c
    char m_pad30[0x1b8 - 0x30];
    i32 m_1b8;     // +0x1b8
    CString m_1bc; // +0x1bc
    i32 m_1c0;     // +0x1c0 preview counter
};

// @early-stop
// 94.7% - entropy tail: the only residual is the cue object `p` getting spilled to
// the stack and reloaded for the final ConfigureItem(this) where retail keeps it in
// eax (the cue temp uses ecx/edi/ebp), plus function-tail nop padding. Logic +
// externs match retail. Final sweep.
RVA(0x000de420, 0x115)
void CPreviewState::LoadLevelPreviewScreen() {
    char buf[64];
    i32 idx = m_1c0;
    m_1c0 = idx + 1;
    sprintf(buf, "PREVIEW%i", idx);
    m_1bc = buf;
    sprintf(buf, "\\SCREENZ\\%s", m_1bc.m_pchData);
    m_2c->ResolveQualified(buf, (void*)0x504358);
    i32 failed = 0;
    if (FadeInTitle(m_1bc.m_pchData, 0, 0, 0, 0, 1) == 0) {
        failed = 1;
    } else {
        CStatusBarHolder* h = m_0c->m_28;
        if (h->m_30 == 0) {
            CueObj* p = 0;
            h->m_10map.Lookup("GAME_TELEPORTEROPEN", &p);
            if (p != 0) {
                i32 tag = g_sndCueTag;
                if (g_sndEnabled != 0 && (u32)(g_killCueClock - p->m_14) >= (u32)p->m_18) {
                    p->m_14 = g_killCueClock;
                    p->m_10->ConfigureItem(tag, 0, 0, 0);
                }
            }
        }
        RetireScene(0x50, 0x3e8, 0, 1);
    }
    m_1b8 = 60000;
    if (failed) {
        Cancel();
    }
}
