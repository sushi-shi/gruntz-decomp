// LevelPreview.cpp - the level-preview screen tick (RVA 0xde420).
//
// Advances the PREVIEW%i counter, resolves the next \SCREENZ\%s namespace, fades
// its title in and (when the live surface is free) plays the teleporter-open cue;
// on a failed fade it cancels the command. Field names are placeholders; only
// offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (operator=(LPCSTR) 0x1b9e74, reloc-masked)
#include <rva.h>

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface + IDirectDrawSurface (IsLost/Flip)
#include <ddraw.h>              // real IDirectDrawSurface dispatch (Mfc.h above supplies windows.h)
#include <DDrawMgr/DDrawSurfacePair.h> // the ONE CDDrawSurfacePair shape (m_surface @+0x2c)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (Method_158b40 @0x158b40)
#include <stdio.h>

#include <Bute/SymTab.h>
#include <Gruntz/SoundCue.h>               // the ONE +0x28 cue holder (CSndHost / CSndEmitter)
#include <Gruntz/StatusBarUpdatersViews.h> // the ONE CRegHolder (CState::m_c world holder)
#include <Gruntz/SoundCueMgr.h>            // the ONE CSoundCueMgr shape (ConfigureItem @0x1360d0)
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (ReportError/DelayedQuit + CGameWnd chain)
#include <Globals.h>

extern "C" {
    DATA(0x0061ab20)
    extern i32 g_sndEnabled;
    DATA(0x0061ab24)
    extern i32 g_sndCueTag;
    DATA(0x006bf3c0)
    extern u32 g_killCueClock;
}

// CPreviewState::m_0c is the ONE world/resource holder (== g_gameReg->m_world ==
// CState::m_c); modeled as CRegHolder (<Gruntz/StatusBarUpdatersViews.h>), shared with
// the status-bar updater TU. The former local PreviewMgr view folded onto it.

SIZE_UNKNOWN(CPreviewState);
class CPreviewState {
public:
    // The screen's vtable (vptr @+0x00); only slot 8 (+0x20) - the per-frame
    // "advance" - is dispatched. The leading slots are placeholders so the index
    // lands at 8; the vtable itself lives in another TU (no ctor here -> none is
    // emitted), so these never need a body.
    virtual i32 Vf0();
    virtual i32 Vf1();
    virtual i32 Vf2();
    virtual i32 Vf3();
    virtual i32 Vf4();
    virtual i32 Vf5();
    virtual i32 Vf6();
    virtual i32 Vf7();
    virtual i32 Advance(); // slot 8 (+0x20)

    i32 Tick();                                                     // 0x0de200
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0
    void RetireScene(i32 a, i32 b, i32 c, i32 d);                   // 0x0fa8f0
    void Cancel();                                                  // 0x0de590
    void LoadLevelPreviewScreen();
    i32 LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3); // 0x0fab90

    CGruntzMgr* m_04; // +0x04  game-manager singleton
    i32 m_08;         // +0x08
    CRegHolder* m_0c; // +0x0c  world/resource holder (== g_gameReg->m_world)
    char m_pad10[0x2c - 0x10];
    CSymTab* m_2c; // +0x2c
    char m_pad30[0x1b8 - 0x30];
    u32 m_1b8;     // +0x1b8 countdown timer
    CString m_1bc; // +0x1bc
    i32 m_1c0;     // +0x1c0 preview counter
};

// CPreviewState::Tick (0x0de200) - the per-frame advance: if the live back surface
// is present and NOT lost, or the screen's Advance virtual fails, report the error
// and bail (returns 0); otherwise tick the audio cue and count the +0x1b8 timer
// down by the frame delta (clamped at 0), returning 1 to keep the screen alive.
//
// @early-stop
// 99.78% - pointer-chain scratch-register wall (docs/patterns/reread-member-view-
// pointer.md): the m_0c->m_28->m_2c audio-cue deref - retail reuses eax for the
// m_28 intermediate (`mov eax,[eax+0x28]`), cl picks ecx; 2 register-field bytes.
// Logic + control flow + all externs byte-exact. Final sweep.
RVA(0x000de200, 0x85)
i32 CPreviewState::Tick() {
    IDirectDrawSurface* surf = m_0c->m_04->m_frontPair->m_surface->m_8;
    if (surf == 0 || surf->IsLost() != 0) {
        if (Advance() == 0) {
            m_04->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }
    SoundStream* snd = m_0c->m_statusBar->m_2c;
    if (snd != 0) {
        snd->PurgeVoiceList(-1);
    }
    if ((u32)g_wap32FrameDelta >= m_1b8) {
        m_1b8 = 0;
    } else {
        m_1b8 = m_1b8 - g_wap32FrameDelta;
    }
    return 1;
}

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
    sprintf(buf, "\\SCREENZ\\%s", (const char*)m_1bc);
    m_2c->ResolveQualified(buf, &g_screenTag);
    i32 failed = 0;
    if (FadeInTitle((char*)(const char*)m_1bc, 0, 0, 0, 0, 1) == 0) {
        failed = 1;
    } else {
        CSndHost* h = m_0c->m_statusBar;
        if (h->m_emitGate == 0) {
            CSndEmitter* p = 0;
            h->m_10.Lookup("GAME_TELEPORTEROPEN", &p);
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

// CPreviewState::LoadScreen (0x0fab90) - resolve the named "\SCREENZ\<name>"
// namespace through the bute symbol table (tagged with g_screenTag), install the
// resolved image into the DirectDraw worker, and - when asked - flip it to the
// front. Returns 0 if any prerequisite (the preview mgr, the +0x08 gate, or the
// symbol table) is missing or any step fails, else 1.
//
// @early-stop
// 96.39% - pointer-chain scratch-register + arg-eval-order wall (docs/patterns/
// pin-local-for-callee-saved-reg.md: "inner-split hurts 2-arg"): the m_0c->m_04
// object base for the 2-arg Method_158b40 call is hoisted before the arg pushes by
// retail (cl emits it after), and the m_0c->m_04->m_frontPair->m_surface Flip chain picks edx/
// eax where cl picks ecx/edx - ~6 register-field bytes. Logic + control flow + all
// externs (sprintf, g_screenTag, ResolveQualified, Method_158b40, Flip) byte-exact.
// Final sweep.
RVA(0x000fab90, 0xaa)
i32 CPreviewState::LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3) {
    if (m_0c == 0) {
        return 0;
    }
    if (m_08 == 0) {
        return 0;
    }
    if (m_2c == 0) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    i32 sym = m_2c->ResolveQualified(buf, &g_screenTag);
    if (sym == 0) {
        return 0;
    }
    if (m_0c->m_04->Method_158b40(sym, 1) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        m_0c->m_04->m_frontPair->m_surface->Flip(0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CPreviewState::Cancel (0x0de590), driven by LoadLevelPreviewScreen (on a failed
// fade) and by the level-preview command router (0x4de3c0, tag 0x1b) - both invoke
// it on the CPreviewState `this` (xref-confirmed). When the global gate (g_flag64c69c)
// is set it delegates to the game mgr's delayed-quit (m_04 == CGruntzMgr, DelayedQuit
// @0x8f530); otherwise it posts WM_COMMAND 0x8027 to the mgr's top window
// (m_04->m_4->m_4). The old PreviewCancelHost/PreviewCancelWnd placeholders were a
// second view of CPreviewState + CGruntzMgr; dissolved onto the real ones.
DATA(0x0024c69c)
extern i32 g_flag64c69c; // DAT_0064c69c
RVA(0x000de590, 0x2e)
void CPreviewState::Cancel() {
    if (g_flag64c69c) {
        m_04->DelayedQuit();
        return;
    }
    PostMessageA((HWND)(m_04->m_gameWnd->m_hwnd), 0x111, 0x8027, 0);
}
