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
#include <Gruntz/State.h>    // the CState base this screen state derives (real vtable)
#include <Gruntz/SoundCue.h> // the ONE +0x28 cue holder (CSndHost / CSndEmitter)
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

// CPreviewState - the level-preview screen state (shows PREVIEW%i, fades \SCREENZ\%s).
// A real CState leaf: proven by the shared base layout (its m_4/m_8/m_c/m_2c coincide
// with CState's, verified vs the other screen states) and the per-frame slot-8 virtual
// Tick dispatches. It has NO distinct vtable of its own - the CState family is RTTI-
// complete - so the four methods below are non-virtual command handlers the game's
// command table dispatches. The former standalone class carried a hand-rolled
// `virtual Vf0..Vf7 + Advance` vtable: that was a FICTION invented only so the
// [vptr+0x20] dispatch would compile (which is why it had no VTBL() to anchor). It is
// dissolved into real CState inheritance here - no phantom vtable.
//
// The per-frame poll Tick dispatches through [vptr+0x20] is CState slot 8, i.e. the
// inherited CState::InputVirtual - Tick just calls it (dispatch resolves whatever
// override the object's runtime vtable holds). We do NOT redeclare it as a CPreviewState
// override: whether the preview provides its own slot-8 body is UNPROVEN (its vtable is
// unlocated). Every reconstructed sibling overrides slot 8, so it likely does too, but
// that is an inference - not asserted here as a declared-only phantom method.
//
// The +0x0c and +0x2c slots are unresolved dual-views (CRegHolder vs CState's
// CSpriteFactoryHolder at m_c; CSymTab vs CResSource at m_2c) - reached with a view-cast
// at each site, exactly as StatusBarUpdaters.cpp does, until those folds land.
//
// FadeInTitle (0xfa1f0) and RetireScene (0xfa8f0) are SHARED CState base methods, not
// CPreviewState's own: the retail caller graph shows 8+ CState-derived siblings
// (CBootyState/CCreditsState/CMulti/CAttract/...) invoke each on their own `this`. They
// are defined under CAttract / CSoundFxEmitter today; declared-only here (reloc-masked
// calls) until their cross-TU re-home onto CState lands.
SIZE_UNKNOWN(CPreviewState);
class CPreviewState : public CState {
public:
    i32 Tick();                                                     // 0x0de200
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0 (CState base method)
    void RetireScene(i32 a, i32 b, i32 c, i32 d);                   // 0x0fa8f0 (CState base method)
    void Cancel();                                                  // 0x0de590
    void LoadLevelPreviewScreen();                                  // 0x0de420
    i32 LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3);         // 0x0fab90

    // CPreviewState-specific fields, past the CState base (which ends at +0x1a8):
    char m_pad1a8[0x1b8 - 0x1a8];
    u32 m_1b8;     // +0x1b8  countdown timer
    CString m_1bc; // +0x1bc  scratch screen-name string (PREVIEW%i / \SCREENZ\%s)
    i32 m_1c0;     // +0x1c0  preview counter
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
    IDirectDrawSurface* surf = ((CRegHolder*)m_c)->m_04->m_frontPair->m_surface->m_8;
    if (surf == 0 || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_4->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }
    SoundStream* snd = ((CRegHolder*)m_c)->m_statusBar->m_2c;
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
    ((CSymTab*)m_2c)->ResolveQualified(buf, &g_screenTag);
    i32 failed = 0;
    if (FadeInTitle((char*)(const char*)m_1bc, 0, 0, 0, 0, 1) == 0) {
        failed = 1;
    } else {
        CSndHost* h = ((CRegHolder*)m_c)->m_statusBar;
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
    if (m_c == 0) {
        return 0;
    }
    if (m_8 == 0) {
        return 0;
    }
    if (m_2c == 0) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    i32 sym = ((CSymTab*)m_2c)->ResolveQualified(buf, &g_screenTag);
    if (sym == 0) {
        return 0;
    }
    if (((CRegHolder*)m_c)->m_04->Method_158b40(sym, 1) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        ((CRegHolder*)m_c)->m_04->m_frontPair->m_surface->Flip(0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CPreviewState::Cancel (0x0de590), driven by LoadLevelPreviewScreen (on a failed
// fade) and by the level-preview command router (0x4de3c0, tag 0x1b) - both invoke
// it on the CPreviewState `this` (xref-confirmed). When the global gate (g_flag64c69c)
// is set it delegates to the game mgr's delayed-quit (m_4 == CGruntzMgr, DelayedQuit
// @0x8f530); otherwise it posts WM_COMMAND 0x8027 to the mgr's top window
// (m_4->m_gameWnd->m_hwnd). The old PreviewCancelHost/PreviewCancelWnd placeholders were a
// second view of CPreviewState + CGruntzMgr; dissolved onto the real ones.
DATA(0x0024c69c)
extern i32 g_flag64c69c; // DAT_0064c69c
RVA(0x000de590, 0x2e)
void CPreviewState::Cancel() {
    if (g_flag64c69c) {
        m_4->DelayedQuit();
        return;
    }
    PostMessageA((HWND)(m_4->m_gameWnd->m_hwnd), 0x111, 0x8027, 0);
}
