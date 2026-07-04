// MenuState.cpp - the front-end menu state (C:\Proj\Gruntz). Trace-discovered
// routines, defined in ascending retail-RVA order:
//   LoadGameOptionsToDialog @0x036860 - the free __cdecl options-dialog writer that
//       snapshots g_gameReg into the g_opt_* staging globals and pushes them into the
//       checkboxes/sliders (inverse of ReadMenuOptionsDialog).
//   ReadMenuOptionsDialog @0x036a30 - the free __cdecl options-dialog reader that
//       commits the menu/audio/video checkbox + slider controls into g_gameReg.
//   CMenuState::ReadyGate  @0x0a0d40 - the &&-chained ready/transition probe.
//
// CMenuState : CState (RTTI .?AVCMenuState@@); the class body + the bulk of its
// methods live in <Gruntz/GameMode.h> / src/Gruntz/GameMode.cpp. This TU adds the
// two trace-discovered routines. Only offsets / control IDs / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (the game-manager singleton; one true shape)

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
