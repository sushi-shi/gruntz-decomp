#include <Mfc.h> // real MFC CString (operator=(LPCSTR) 0x1b9e74, reloc-masked)
#include <Gruntz/LeafCue.h>
#include <rva.h>

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface + IDirectDrawSurface (IsLost/Flip)
#include <ddraw.h>              // real IDirectDrawSurface dispatch (Mfc.h above supplies windows.h)
#include <DDrawMgr/DDrawSurfacePair.h> // the ONE CDDrawSurfacePair shape (m_surface @+0x2c)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (LoadPageImage @0x158b40)
#include <stdio.h>

#include <Bute/SymTab.h>
#include <Bute/SymParser.h>               // CSymParser::ResolvePath (0x13c030) on m_8
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan::ScanTree (0x157ee0)
#include <Wap32/Wap32.h>                  // CGameWnd::PumpMessages (0x13d4e0)
#include <Gruntz/State.h>                 // the CState base this screen state derives (real vtable)
#include <Gruntz/SoundCue.h>        // the ONE +0x28 cue holder (CDDrawSubMgrLeafScan / LeafCue)
#include <Gruntz/GameRegistry.h>    // CDDrawSurfaceMgr (the typed CState::m_c holder)
#include <Dsndmgr/DirectSoundMgr.h> // the ONE DSoundCloneInst shape (ConfigureItem @0x1360d0)
#include <Dsndmgr/SoundStream.h>    // SoundStream::Stop (ResetPreview's owned stream)
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (ReportError/DelayedQuit + CGameWnd chain)
#include <Gruntz/PreviewState.h> // canonical CPreviewState (the level-preview screen state)
#include <Rez/FrameClock.h>      // frame-clock band (g_killCueClock)
#include <Rez/RezSync.h>         // ex Globals.h
#include <Wap32/GameApp.h>       // ex Globals.h
#include <Gruntz/SoundState.h>   // ex Globals.h transitive
#include <Gruntz/LevelPreview.h> // ex Globals.h

DATA(0x00104358)
i32 g_screenTag;

RVA(0x000de030, 0xc2)
i32 CPreviewState::Enter(void* mgr, i32 a1, i32 a2) {
    // The base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9;
    // CState::LoadGameAssetNamespaces is the slot-1 virtual now, and retail calls
    // the default body direct here).
    if (CState::LoadGameAssetNamespaces(reinterpret_cast<i32>(mgr), a1, a2) == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_PREVIEW"));
    if (m_2c == 0) {
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSound == 0) {
        void* set = SymTab2c()->FindSub("SOUNDZ");
        if (set != 0) {
            m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(set), "PREVIEW", "_");
        }
    }
    m_1bc = "PREVIEW0";
    m_1c0 = 0;
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

// CPreviewState::ResetPreview (0x0de140) - the preview teardown: stop the owned sound
// stream, prune the PREVIEW-prefixed sound-registry keys, then chain the CState base
// teardown (qualified -> direct call to CState::ReleaseResources @0xfa150).
// ATTRIBUTION (ex "CGameModeBase::ResetPreview"; homed here from GameMode.cpp): the
// body sits INSIDE this TU's retail obj block (between Enter @0xde030 and
// NextScreenCmd @0xde190) and reads only CState-level fields - but retail has ZERO
// references to it (no vtable slot, no call, no thunk ref; whole-image scan
// 2026-07-16), i.e. it is linked-in dead code, so it is a plain non-virtual here -
// NOT asserted as this class's slot-2 override (CPreviewState's vtable is unlocated).
// m_c->m_soundRegistry is re-read each statement (retail does not cache it).
// @early-stop
// ~98.8% - m_28-intermediate regalloc wall (retail reuses eax->eax->ecx; cl picks
// fresh ecx/edx) - a 2-3 byte modrm micro-diff, not source-steerable.
RVA(0x000de140, 0x33)
void CPreviewState::ResetPreview() {
    if (m_world->m_soundRegistry->m_2c != 0) {
        m_world->m_soundRegistry->m_2c->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("PREVIEW", "_");
    CState::ReleaseResources();
}

RVA(0x000de190, 0x35)
i32 CPreviewState::NextScreenCmd(i32 param) {
    while (ShowCursor(FALSE) >= 0) {
    }
    LoadLevelPreviewScreen();
    m_1b8 = 60000;
    return 1;
}

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
    IDirectDrawSurface* surf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (surf == 0 || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }
    SoundStream* snd = m_world->m_soundRegistry->m_2c;
    if (snd != 0) {
        snd->PurgeVoiceList(-1);
    }
    if (static_cast<u32>(g_wap32FrameDelta) >= m_1b8) {
        m_1b8 = 0;
    } else {
        m_1b8 = m_1b8 - g_wap32FrameDelta;
    }
    return 1;
}

RVA(0x000de2c0, 0x5c)
i32 CPreviewState::Refade() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = FadeInTitle(const_cast<char*>(static_cast<const char*>(m_1bc)), 0, 0, 0, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    return r;
}

RVA(0x000de340, 0x56)
i32 CPreviewState::RefadeVirtual() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = FadeInTitle(const_cast<char*>(static_cast<const char*>(m_1bc)), 0, 0, 0, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    return r;
}

RVA(0x000de3c0, 0x2d)
i32 CPreviewState::OnKey(i32 key, i32 param) {
    if (key == 0x1b) {
        Cancel();
    }
    if (key == 0x20 || key == 0xd) {
        LoadLevelPreviewScreen();
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
    sprintf(buf, "\\SCREENZ\\%s", static_cast<const char*>(m_1bc));
    SymTab2c()->ResolveQualified(buf, &g_screenTag);
    i32 failed = 0;
    if (FadeInTitle(const_cast<char*>(static_cast<const char*>(m_1bc)), 0, 0, 0, 0, 1) == 0) {
        failed = 1;
    } else {
        CDDrawSubMgrLeafScan* h = m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            void* p_ob = 0;
            h->m_10.Lookup("GAME_TELEPORTEROPEN", p_ob);
            LeafCue* p = static_cast<LeafCue*>(p_ob);
            if (p != 0) {
                i32 tag = g_sndCueTag;
                if (g_sndEnabled != 0
                    && static_cast<u32>((g_killCueClock - p->m_14)) >= static_cast<u32>(p->m_18)) {
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
// object base for the 2-arg LoadPageImage call is hoisted before the arg pushes by
// retail (cl emits it after), and the m_0c->m_04->m_frontPair->m_surface Flip chain picks edx/
// eax where cl picks ecx/edx - ~6 register-field bytes. Logic + control flow + all
// externs (sprintf, g_screenTag, ResolveQualified, LoadPageImage, Flip) byte-exact.
// Final sweep.
RVA(0x000fab90, 0xaa)
i32 CPreviewState::LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3) {
    if (m_world == 0) {
        return 0;
    }
    if (m_symParser == 0) {
        return 0;
    }
    if (m_2c == 0) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    CParseSource* sym = SymTab2c()->ResolveQualified(buf, &g_screenTag);
    if (sym == 0) {
        return 0;
    }
    if (m_world->m_drawTarget->LoadPageImage(sym, 1) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    }
    return 1;
}

DATA(0x0024c69c)
i32 g_flag64c69c = 0; // DAT_0064c69c  (owner-TU definition)
RVA(0x000de590, 0x2e)
void CPreviewState::Cancel() {
    if (g_flag64c69c) {
        m_mgr->DelayedQuit();
        return;
    }
    PostMessageA(static_cast<HWND>((m_mgr->m_gameWnd->m_hwnd)), 0x111, 0x8027, 0);
}
