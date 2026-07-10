// MenuState.cpp - CMenuState, the front-end menu game-state (C:\Proj\Gruntz).
// Split out of the former GameMode.cpp god-TU (per-class TU cut): CMenuState now owns
// its full method set here; the CState base + the CGameModeBase cleanup pair stay in
// GameMode.cpp, the sibling states (CCreditsState/CBootyState/CMultiBootyState) in
// their own TUs. The ~CMenuState `??1` (with the CState ctor) is the class's vtable +
// inline-virtual (Update) emission anchor - it stays in this TU with the rest of
// CMenuState. Its MENU asset loader (LoadAssets @0x9fe50) lives in MenuStateAssets.cpp.
//
// Functions, ascending retail-RVA order:
//   CMenuState::FormatHudText @0x01af70 - the 960-B HUD-text formatter switch.
//   LoadGameOptionsToDialog   @0x036860 - free __cdecl options-dialog writer.
//   ReadMenuOptionsDialog     @0x036a30 - free __cdecl options-dialog reader.
//   OnToggle*Option           @0x036d00.. - per-checkbox WM_COMMAND handlers.
//   ~CMenuState / ReleaseResources / StartMusic / StopMusicChain / FrameSlot28 /
//   Render / Vslot0c / Vslot0e / Vslot10  @0x08ce60.. - teardown + per-frame draw.
//   CMenuState::ReadyGate     @0x0a0d40 - the &&-chained ready/transition probe.
//   CMenuState::BuildVersionString @0x0a0d80 - the on-screen version banner.
//
// CMenuState : CState (RTTI .?AVCMenuState@@); the class body lives in
// <Gruntz/GameMode.h>. Only offsets / control IDs / code bytes are load-bearing;
// names are placeholders for the recovered engine identities.
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntzMgr.h>            // CGruntzMgr (the game-manager singleton; one true shape)
#include <Gruntz/SoundCueMgr.h>         // CSoundCueMgr (StartMusic/StopMusicChain ConfigureItem)
#include <Gruntz/WwdGameReg.h>          // g_gameReg (StartMusic music gate)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources leaf keys)
#include <DDrawMgr/DDrawSubMgrPages.h>  // CDDrawSubMgrPages (FrameSlot28 flush)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was a GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>         // CDDSurface Flip (FrameSlot28)

#include <rva.h>
#include <Win32.h> // IsDlgButtonChecked + HWND (real USER32 header)
#include <Globals.h>

// ---------------------------------------------------------------------------
// The game-manager singleton (*g_64556c) the options reader commits into - the
// SAME object + helper set CPlay::ApplyGameOptions (0x036be0) writes. The option
// setters reach the input/audio sub-systems through the manager: the "apply music
// option" commit (SetRunState @0x092340, via the ILT thunk 0x492340), the two
// slider commits (StoreInputFlag @0x0919d0 / StoreInputState @0x091a10, thunks
// 0x4919d0/0x491a10), the "apply speech option" commit (SetSoundLevelState
// @0x0923b0, thunk 0x4923b0), and the m_sound object's XMIDI master-volume
// push/read (0x138950/0x1389c0). All reloc-masked; the one true CGruntzMgr shape
// lives in <Gruntz/GruntzMgr.h>.
extern "C" {
    DATA(0x0024556c)
    extern CGruntzMgr* g_mgrSettings; // = g_64556c (the CGruntzMgr singleton)
    DATA(0x0020ccc4)
    extern i32 g_videoResolutionMode;
    // The audio-mode gate globals (.data ints) the reader branches on: the master
    // mute (0x6455b4), the music-block disable (0x6455bc), the speech-block disable
    // (0x6455c0) - the SAME gates CPlay::ApplyGameOptions tests.
    DATA(0x002455b4)
    extern i32 g_gate_2455b4;

    // The options-dialog staging globals (.data ints): LoadGameOptionsToDialog
    // (0x036860) snapshots g_gameReg into them then pushes them into the controls;
    // CPlay::ApplyGameOptions (0x036be0) reads them back. SAME g_opt_* set CPlay names.
}

// Screen-resolution detector (0x036f30-2 sibling at 0x0363a0): reads g_gameReg's
// screen width/height (+0x94/+0x98) and returns the mode (3=1024x768, 2=800x600,
// else 1). A free __cdecl no-arg fn (loads its own g_gameReg, ignores ecx).
i32 DetectScreenResMode(); // 0x0363a0

// The combo dialog-population helper (external/no-body so the call rel32
// reloc-masks). LoadVideoResolutionConfig seeds the resolution combo.
void LoadVideoResolutionConfig(void* hDlg, i32 nIDCombo, i32 nSel); // 0x036f30

// The dialog slider-fill setter (0x0371e0) + scroll-position getter (0x036ec0)
// every options slider funnels through - forward-declared here (the callers
// precede them in retail-RVA order); DEFINED below after ReadMenuOptionsDialog.
namespace ApiCallerStubs {
    void winapi_0371e0_GetDlgItem_SetScrollInfo(HWND hDlg, i32 id, i32 pos, i32 max); // 0x0371e0
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(HWND hDlg, i32 id);                    // 0x036ec0
} // namespace ApiCallerStubs

// ===========================================================================
// CMenuState per-frame + teardown methods (moved from the former GameMode.cpp
// god-TU). The ~CMenuState `??1` anchors the CMenuState vtable/inline-virtual
// emission in this TU.
// ===========================================================================

// FormatHudText's stats source: g_mgrSettings->m_scoreHud (the +0x7c CBattlezData
// HUD/score accumulator) viewed for its 13 live-value getters (thiscall on the stats
// object) + cached fields. The getter path is gated by m_liveGame && stats->m_c (the
// sibling-guard idiom). Fold onto CBattlezData is a follow-up (placeholder getters).
SIZE_UNKNOWN(CHudStats);
struct CHudStats {
    i32 GetC10();
    i32 GetC1c();
    i32 GetC20();
    i32 GetC34();
    i32 GetC18();
    i32 GetC30();
    i32 GetC14();
    i32 GetC38();
    i32 GetC24();
    i32 GetC40();
    i32 GetC2c();
    i32 GetC3c();
    i32 GetC28();
    char p0[0xc];
    i32 m_c; // +0xc  live-game flag (getter gate)
    i32 m_10, m_14, m_18, m_1c, m_20, m_24, m_28, m_2c, m_30, m_34, m_38, m_3c, m_40;
};
#define STATS ((CHudStats*)g_mgrSettings->m_scoreHud)
#define STAT(getter, field) ((m_liveGame != 0 && STATS->m_c != 0) ? STATS->getter() : STATS->field)

// CMenuState::FormatHudText(buf, sel) (0x1af70): the 960-byte HUD-text formatter - an
// 8-case switch that sprintf()s the game clock (MM:SS via the imul-by-0x10624dd3
// divide-by-1000 then /60), score, and "%d of %d" progress into `buf`. Every stat is
// read via STAT(getter, field). The default case writes "???".
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// 960-byte switch body is CODE-BYTE-EXACT (verified llvm-objdump -dr base vs retail:
// every stat sibling-guard block, the MM:SS unsigned /1000-then-/60 divide magic, the
// "%d of %d" clamp, the 13 stats-thiscall getters, and the sprintf pushes all match;
// the ~24 g_mgrSettings loads are the retail A1 moffs32 form). Residual ~2.5% is the
// inline .rdata jump table (8 case addresses) + the reloc-typed format-string DIR32
// operands, neither source-steerable. ~97.5%.
RVA(0x0001af70, 0x3c0)
void CMenuState::FormatHudText(CString* buf, i32 sel) {
    switch (sel) {
        case 0: {
            u32 secs = (u32)(STAT(GetC10, m_10) / 1000);
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case 1:
            buf->Format("%d", STAT(GetC1c, m_1c));
            return;
        case 2:
            buf->Format("%d", STAT(GetC20, m_20));
            return;
        case 3: {
            i32 total = STAT(GetC34, m_34);
            i32 cap = STAT(GetC34, m_34);
            i32 cur = STAT(GetC18, m_18);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 4: {
            i32 total = STAT(GetC30, m_30);
            i32 cap = STAT(GetC30, m_30);
            i32 cur = STAT(GetC14, m_14);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 5: {
            i32 total = STAT(GetC38, m_38);
            i32 cap = STAT(GetC38, m_38);
            i32 cur = STAT(GetC24, m_24);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 6: {
            i32 total = STAT(GetC40, m_40);
            i32 cap = STAT(GetC40, m_40);
            i32 cur = STAT(GetC2c, m_2c);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 7: {
            i32 total = STAT(GetC3c, m_3c);
            i32 cap = STAT(GetC3c, m_3c);
            i32 cur = STAT(GetC28, m_28);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        default:
            *buf = "???";
            return;
    }
}

// LoadGameOptionsToDialog @0x036860 - the inverse of ReadMenuOptionsDialog: snapshot
// the live g_gameReg settings into the g_opt_* staging globals (so a later Cancel can
// restore / Apply can commit them), then push each value into the options dialog's
// checkboxes (CheckDlgButton via the cached IAT slot 0x6c44b4) and sliders
// (winapi_0371e0). The video-resolution combo + caption go through
// LoadVideoResolutionConfig. A free __cdecl function (no `this`); bails if no manager.
RVA(0x00036860, 0x16f)
void LoadGameOptionsToDialog(HWND hDlg) {
    if (g_mgrSettings == 0) {
        return;
    }
    g_opt_22bd70 = g_mgrSettings->m_isEasyMode;
    g_opt_22bd6c = g_mgrSettings->m_inputFlag;
    g_opt_22bd84 = g_mgrSettings->m_soundEnabled;
    g_opt_22bdc4 = g_mgrSettings->m_inputStateVal;
    g_opt_22bdd4 = g_mgrSettings->m_isVoiceEnabled;
    g_opt_22bdcc = g_mgrSettings->m_sound->GetXMidiVolume();
    g_opt_22bdd0 = g_mgrSettings->m_musicEnabled;
    g_opt_22bd68 = g_mgrSettings->m_scrollSpeed;
    g_opt_22bd64 = g_mgrSettings->m_musicEnabled;
    g_opt_22bdc8 = DetectScreenResMode();
    g_videoResolutionMode = DetectScreenResMode();

    CheckDlgButton(hDlg, 0x455, g_mgrSettings->m_isEasyMode);
    LoadVideoResolutionConfig(hDlg, 0x52c, g_videoResolutionMode);
    CheckDlgButton(hDlg, 0x46d, g_mgrSettings->m_soundEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x470,
        g_mgrSettings->m_inputFlag,
        0x50
    );
    CheckDlgButton(hDlg, 0x475, g_mgrSettings->m_isVoiceEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x476,
        g_mgrSettings->m_inputStateVal,
        0x50
    );
    CheckDlgButton(hDlg, 0x471, g_mgrSettings->m_musicEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x472,
        g_mgrSettings->m_sound->GetXMidiVolume(),
        0x64
    );
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x478,
        g_mgrSettings->m_scrollSpeed,
        0x64
    );
}

// ReadMenuOptionsDialog @0x036a30 - commit the front-end options dialog's control
// state into the game-manager settings: the master-enable checkbox (0x455), the
// video-resolution slider (0x52c, clamped 0..100), and - unless globally muted -
// the music/sound block (0x46d/0x470/0x475/0x476) and, when the speech channel is
// present, the speech block (0x471/0x472). The trailing master/quality slider
// (0x478) is always committed. A free __cdecl function (no `this`). The SAME
// commit idiom as CPlay::ApplyGameOptions, with dialog reads as the value source.
//
// @early-stop
// entropy tail (~99.9%): every instruction byte matches retail except (a) the
// IsDlgButtonChecked IAT reference - retail names the cached IAT pointer by its
// fixed VA 0x6c44b0, our <Win32.h> import emits the identical `mov edi,ds:[__imp]`
// against __imp__IsDlgButtonChecked@8 (win32-import scoring artifact, same bytes),
// and (b) a single ecx-vs-edx coloring of the m_100 manager reload (regalloc
// coin-flip). No real code divergence; no source lever flips either.
RVA(0x00036a30, 0x14e)
void ReadMenuOptionsDialog(HWND hDlg) {
    if (g_mgrSettings == 0) {
        return;
    }
    g_mgrSettings->m_isEasyMode = IsDlgButtonChecked(hDlg, 0x455);
    i32 res = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x52c);
    if (res >= 0 && res <= 100) {
        g_videoResolutionMode = res;
    }
    if (g_gate_2455b4 == 0) {
        if (g_gate_2455bc == 0) {
            g_mgrSettings->SetRunState(IsDlgButtonChecked(hDlg, 0x46d));
            i32 mv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x470);
            if (mv >= 0 && mv <= 100) {
                g_mgrSettings->StoreInputFlag(mv);
            }
            g_mgrSettings->m_isVoiceEnabled = IsDlgButtonChecked(hDlg, 0x475);
            i32 sv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x476);
            if (sv >= 0 && sv <= 100) {
                g_mgrSettings->StoreInputState(sv);
            }
        }
        if (g_gate_2455b4 == 0 && g_gate_2455c0 == 0 && g_mgrSettings->m_sound->m_enabled != 0) {
            g_mgrSettings->SetSoundLevelState(IsDlgButtonChecked(hDlg, 0x471));
            i32 pv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x472);
            if (pv >= 0 && pv <= 100) {
                g_mgrSettings->m_sound->SetXMidiVolume(pv);
            }
        }
    }
    i32 qv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x478);
    if (qv >= 0 && qv <= 100) {
        g_mgrSettings->m_scrollSpeed = qv;
    }
}

// The four per-checkbox WM_COMMAND handlers of the same options dialog: each
// mirrors one checkbox into the CGruntzMgr settings singleton and enables the
// paired slider/control. Free __cdecl(hWnd); no-op when the manager is not live.
// SAME control IDs + commit set as ReadMenuOptionsDialog above (the reader's
// per-control split-out). g_mgrSettings loads reloc-mask; the commit calls
// (SetRunState/SetSoundLevelState) go through the thunks 0x492340/0x4923b0.

// 0x36d00: music-enable checkbox (0x46d) -> SetRunState, enable slider 0x470.
RVA(0x00036d00, 0x40)
void OnToggleMusicOption(HWND hWnd) {
    if (g_mgrSettings) {
        i32 state = IsDlgButtonChecked(hWnd, 0x46d);
        g_mgrSettings->SetRunState(state);
        EnableWindow(GetDlgItem(hWnd, 0x470), state);
    }
}

// 0x36d50: voice-enable checkbox (0x475) -> m_isVoiceEnabled, enable ctrl 0x476.
RVA(0x00036d50, 0x3c)
void OnToggleVoiceOption(HWND hWnd) {
    if (g_mgrSettings) {
        i32 checked = IsDlgButtonChecked(hWnd, 0x475);
        g_mgrSettings->m_isVoiceEnabled = checked;
        EnableWindow(GetDlgItem(hWnd, 0x476), checked);
    }
}

// 0x36da0: speech-enable checkbox (0x471) -> SetSoundLevelState, enable ctrl 0x472.
RVA(0x00036da0, 0x40)
void OnToggleSpeechOption(HWND hWnd) {
    if (g_mgrSettings) {
        i32 state = IsDlgButtonChecked(hWnd, 0x471);
        g_mgrSettings->SetSoundLevelState(state);
        EnableWindow(GetDlgItem(hWnd, 0x472), state);
    }
}

// 0x36e10: easy-mode checkbox (0x455) -> m_isEasyMode.
RVA(0x00036e10, 0x26)
void OnToggleEasyModeOption(HWND hWnd) {
    if (g_mgrSettings) {
        g_mgrSettings->m_isEasyMode = IsDlgButtonChecked(hWnd, 0x455);
    }
}

namespace ApiCallerStubs {
    // __cdecl(hDlg, id, pos): set dialog item `id`'s scroll position (SIF_POS only,
    // redraw). The 3-arg sibling of winapi_0371e0 (which also sets range/page).
    RVA(0x00036e50, 0x43)
    void winapi_036e50_GetDlgItem_SetScrollPos(HWND hDlg, i32 id, i32 pos) {
        HWND h = GetDlgItem(hDlg, id);
        if (h) {
            SCROLLINFO si;
            si.cbSize = 0x1c;
            si.fMask = SIF_POS;
            si.nPos = pos;
            SetScrollInfo(h, SB_CTL, &si, TRUE);
        }
    }

    // __cdecl(hDlg, id): read the scroll position of dialog item `id`.
    RVA(0x00036ec0, 0x41)
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(HWND hDlg, i32 id) {
        HWND h = GetDlgItem(hDlg, id);
        if (!h) {
            return 0;
        }
        SCROLLINFO si;
        si.cbSize = 0x1c;
        si.fMask = SIF_POS;
        GetScrollInfo(h, SB_CTL, &si);
        return si.nPos;
    }

    // __cdecl(hDlg, id, pos, max): set dialog item `id`'s scroll range/page/pos.
    RVA(0x000371e0, 0x5b)
    void winapi_0371e0_GetDlgItem_SetScrollInfo(HWND hDlg, i32 id, i32 pos, i32 max) {
        HWND h = GetDlgItem(hDlg, id);
        if (h) {
            SCROLLINFO si;
            si.nMax = max;
            si.cbSize = 0x1c;
            si.fMask = 0x17;
            si.nMin = 1;
            si.nPage = 0xa;
            si.nPos = pos;
            SetScrollInfo(h, SB_CTL, &si, FALSE);
        }
    }
} // namespace ApiCallerStubs

// ---------------------------------------------------------------------------
// CMenuState teardown + per-frame draw (moved from GameMode.cpp). The ~CMenuState
// `??1` anchors the CMenuState vtable/inline-virtual emission in this TU.
// ---------------------------------------------------------------------------

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use).
static inline CGMOwner* Owner(CState* s) {
    return (CGMOwner*)s->m_4;
}

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// The renderer's DisposeWorkers @0x163c60 IS CDDrawWorkerList::ClearWorkers; local decl
// (CDDrawWorkerList has no shared header - defined in src/DDrawMgr/DDrawWorkerList.cpp).
class CDDrawWorkerList {
public:
    void ClearWorkers();
};

// The menu music controller (CMenuState+0x1bc): a player @+0x10 (real DirectSoundMgr,
// IsPlaying 0x1353f0 / CloneAndPlay 0x135660) with a draw-clock gate (last @+0x14,
// interval @+0x18). ConfigureItem is dispatched via CSoundCueMgr.
class DirectSoundMgr;
SIZE_UNKNOWN(CMenuMusic);
struct CMenuMusic {
    char m_pad00[0x10];
    DirectSoundMgr* m_10; // +0x10  player (real DirectSoundMgr)
    i32 m_14;             // +0x14  last draw-clock
    i32 m_18;             // +0x18  interval
};

// The draw-clock mirror + the reentrancy gate the menu music poll save/restores.
extern "C" u32 g_6bf3c0; // draw-clock mirror
extern i32 g_61ab20;     // DAT_0061ab20 reentrancy gate

// StartMusic reads the game registry through its WwdGameReg view (m_10 presence gate,
// m_11c configured item); same 0x24556c singleton as g_mgrSettings, typed WwdGameReg.
extern WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A (reloc-masked)

// CMenuState::~CMenuState() (`??1`, 0x8ce60): run the menu teardown then chain the base.
// ReleaseResources/the base ~CState are statically bound in the dtor.
RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

// CMenuState::ReleaseResources() (slot 2 / +0x8): release the MENU resource set
// (name registry + leaf registry), dispose the worker list, free the menu UI
// object, then chain BaseCleanup. Also reached directly from ~CMenuState.
RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {
    // m_c re-read for each access (retail does not cache it); the null-guarded
    // block tests m_c once and reuses it for both the Free and DisposeWorkers.
    ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("MENU", "_");
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("MENU", "_");
    if (m_c) {
        // The test value of m_c is reused for the leaf-registry access; the
        // worker-list dispose re-reads m_c fresh (retail does not cache it).
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            ((SoundStream*)r)->Stop();
        }
        ((CDDrawWorkerList*)m_c->m_rendererB)->ClearWorkers();
    }
    // m_1b4 IS cached (retail holds it in edi across the pre-delete + delete).
    CChatBox* ui = m_1b4;
    if (ui) {
        ui->~CChatBox();
        operator delete(ui);
        m_1b4 = 0;
    }
    ((CGameModeBase*)this)->BaseCleanup();
}

// CMenuState::StartMusic() (0xa05a0): if the menu music + the registry gate are
// live, push the configured item into the player on the draw-clock window, under
// a save/restored reentrancy gate.
RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_1bc == 0) {
        return;
    }
    if (g_gameReg->m_10 == 0) {
        return;
    }
    i32 saved = g_61ab20;
    i32 flag = saved;
    if (!saved) {
        flag = 1;
        g_61ab20 = 1;
    }
    i32 item = g_gameReg->m_11c;
    CMenuMusic* mus = m_1bc;
    if (flag) {
        u32 clk = g_6bf3c0;
        if (clk - mus->m_14 >= (u32)mus->m_18) {
            mus->m_14 = clk;
            ((CSoundCueMgr*)mus->m_10)->ConfigureItem(item, 0, 0, 1);
        }
    }
    if (!saved) {
        g_61ab20 = saved;
    }
}

// CMenuState::StopMusicChain() (0xa0640): if the menu music is playing, request
// a fade-out stop, then spin the cursor/anim tick until playback ends.
RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_1bc == 0) {
        return;
    }
    CMenuMusic* mus = m_1bc;
    if (!((DirectSoundMgr*)mus->m_10)->IsPlaying()) {
        return;
    }
    m_1bc->m_10->CloneAndPlay(0, 0x1f4, 1);
    if (!m_1bc->m_10->IsPlaying()) {
        return;
    }
    do {
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            ((SoundDevice*)r)->PurgeVoiceList(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

// CMenuState::FrameSlot28(int) (slot 10 / +0x28, 0xa06d0): flush + flip the menu
// view, stamp the start clock, run the music-stop chain, then busy-wait m_1b8 ms.
RVA(0x000a06d0, 0x5f)
i32 CMenuState::FrameSlot28(i32) {
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    m_c->m_drawTarget->m_10->m_2c->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

// CMenuState::Render(): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_645584); m_1b4->Pre(); DrawVersion({g_645cc8..d4});
//      m_1b4->Post();   return 1;
RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    CGMEntityList* L = g_645574;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (i32 i = 0; i < L->m_count; i++) {
        L->m_elems[i]->Update();
    }

    // six prioritized entity-flag scans, each firing a distinct UI handler
    i32 c;
    L = g_645574;
    i32 n = L->m_count;
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x80000000) {
            m_1b4->OnFlag80000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x40000000) {
            m_1b4->OnFlag40000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x20000000) {
            m_1b4->OnFlag20000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_elems[c]->m_2ac & 0x10000000) {
            m_1b4->OnFlag10000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x3) {
            m_1b4->OnFlag00000003();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100()) {
                PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:
    m_1b4->Step(g_645584);
    m_1b4->Pre();
    DrawVersion(g_645cc8);
    m_1b4->Post();
    return 1;
}

// CMenuState::Vslot0c @0xa0b90 (slot 12) - the front-end keydown dispatcher: the
// arrow keys drive the menu UI object's HitTest nav, RETURN/SPACE its Activate, and
// ESCAPE its page Switch(1); when ESCAPE's Switch returns 0 (top page) it clears the
// fade duration and posts WM_COMMAND(0x8027) to the app window. Always returns 1.
RVA(0x000a0b90, 0xc7)
i32 CMenuState::Vslot0c(i32 key, i32 arg2) {
    if (key == 0x28) {
        m_1b4->HitTest2();
    } else if (key == 0x26) {
        m_1b4->HitTest1();
    } else if (key == 0x27) {
        m_1b4->HitTest4();
    } else if (key == 0x25) {
        m_1b4->HitTest3();
    } else if (key == 0xd || key == 0x20) {
        m_1b4->OnFlag00000003();
    } else if (key == 0x1b) {
        if (m_1b4->OnFlag00000100() == 0) {
            m_1b8 = 0;
            PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

// CMenuState slot 14 (+0x38) / slot 16 (+0x40): the two mouse hit-test forwarders -
// each passes (arg2, arg3) to the menu UI object's HitTest0 (== CChatBox::HitTest0),
// discards its result, and returns 1. arg1 (the message/event id) is unused.
RVA(0x000a0ca0, 0x21)
i32 CMenuState::Vslot0e(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}
RVA(0x000a0ce0, 0x21)
i32 CMenuState::Vslot10(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}

// CMenuState::ReadyGate @0x0a0d40 - the &&-chained ready/transition probe: poll
// the active/ready gate (Vfunc3, slot 3); if ready, attempt the pending state
// commit (CommitState, the 0x1136 thunk); if that succeeds, run the activation
// poll (Vslot06, slot 6). A short-circuit chain - each early bail returns the
// failing call's (zero) result.
RVA(0x000a0d40, 0x24)
i32 CMenuState::ReadyGate() {
    i32 r = Vfunc3();
    if (r == 0) {
        return r;
    }
    r = CommitState();
    if (r == 0) {
        return r;
    }
    return Vslot06();
}

// CMenuState::BuildVersionString (0xa0d80): format the on-screen version banner
// into a transient CString, append " (SPAWN MODE)" when the CD prompt latched the
// spawn install, then hand it to the shared HUD-message sprite helper into the
// caller-supplied RECT (the 4 args form the RECT by value). The build/patch field
// g_65160c selects the two- vs three-number version format.
extern "C" i32 g_651608; // 0x651608  version field A
extern "C" i32 g_65160c; // 0x65160c  build/patch field (0 -> two-number format)
extern "C" i32 g_651610; // 0x651610  version field B
// The shared HUD message-sprite helper (0x1154b0 via the 0x1f00 ILT thunk,
// __cdecl): push a transient text sprite carrying `text` into `rect`.
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
RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(i32 rectLeft, i32, i32, i32) {
    CString str;
    if (g_65160c == 0) {
        str.Format("Gruntz v%d.%d", g_651608, g_651610);
    } else {
        str.Format("Gruntz v%d.%d%d", g_651608, g_65160c, g_651610);
    }
    if (g_cdPromptResult) {
        str += " (SPAWN MODE)";
    }
    ShowHudMessage(m_c, &str, (RECT*)&rectLeft, 0x64, 1, 0xff, 0xff, 0, 0);
}
