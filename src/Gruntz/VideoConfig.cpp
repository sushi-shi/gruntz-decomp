// VideoConfig.cpp - original TU: options dialogs (@identity-TODO), interval
// 0x0363a0-0x037900. ONE original TU per docs/exe-map/interval-dossiers.md #10c:
// our videoconfig + gameoptionsdialog units + the NINE menustate dialog helpers
// inside the interval were slices of this single file - the weave is
// videoconfig | gameoptionsdialog | menustate | play | menustate | videoconfig |
// menustate | gameoptionsdialog | videoconfig, and the menustate 27-frag init run
// @0x35c40 directly precedes the interval. The REAL CMenuState core (0xa02c0+,
// with its own frag runs) stays in MenuState.cpp.
//
// Contents (C:\Proj\Gruntz): GameOptionsDlgProc + VideoOptionsDlgProc + their
// load/save/scroll/toggle helpers, the video-resolution combo pair, and
// CPlay::ApplyGameOptions (dossier seam 0x36be0 - applies the dialog's staged
// settings; @identity-TODO whether it is really a CPlay member or a free/dialog
// helper).
//
// Only offsets / control IDs / code bytes are load-bearing; names are placeholders.
#include <Mfc.h> // afx.h FIRST (before any windows.h): GameRegistry.h/GruntzMgr.h pull MFC
#include <Gruntz/GameRegMfcPtr.h>
#include <rva.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr - the 0x24556c settings singleton's one true shape
#include <Gruntz/Play.h>      // CPlay (ApplyGameOptions host)
#include <Gruntz/State.h>     // CState::Update (m_curState state probe)
#include <Gruntz/LeafCue.h>   // LeafCue (the config-cue leaf: m_10/m_14/m_18) for ScrollDialog
#include <Net/NetMgr.h>
#include <Gruntz/Multi.h> // CMulti::SendChannelStat422/423 (dispatched on m_curState; netmgr-vs-cmulti split)
#include <Gruntz/Wnd.h>   // the real MFC CWnd via <afxwin.h> (FromHandle; m_hWnd @+0x1c)
#include <Gruntz/Enums.h> // RES_640x480/RES_800x600/RES_1024x768
#include <Globals.h>      // the g_opt_* staging globals
#include <string.h>       // strcat (inline repnz scasb + rep movs under /O2 /Oi)

// Control-ID literal (kept local, not from <windows.h>). Enum VALUE lowers to the same
// immediate in int context (the GetDlgItem control-id arg); the enum TYPE is not used as
// a parameter type, so mangling is unaffected.
typedef enum VideoConfigDlgId {
    IDC_RESCAPTION = 0x52d, // the "current resolution" static text ctrl
} VideoConfigDlgId;

// ---------------------------------------------------------------------------
// The global selected-resolution discriminator (an int; 1=640x480, 2=800x600,
// 3=1024x768). Save stores the combo's current selection here; Load reads it to
// pick the resolution suffix string; the dialog commit reads it back into the
// manager's saved mode. The reloc that names it is masked in objdiff.
// ---------------------------------------------------------------------------
DATA(0x0020ccc4)
i32 g_videoResolutionMode = 1; // retail .data initial value 1

// ---------------------------------------------------------------------------
// MFC controls reached by call-rel32 (library fns, so the call displacements
// reloc-mask). The combo is wrapped through MFC's CWnd::FromHandle (a static
// __stdcall permanent/temporary-map lookup that returns a CWnd*), then driven as
// the REAL <afxcmn.h> CSliderCtrl: SetRange(min,max,redraw) is genuinely
// OUT-OF-LINE in MFC 4.2 (not in AFXCMN.INL - retail calls the NAFXCW copy
// @0x11e0f9), and the engine 0x405/0x400 messages are exchanged with the
// wrapped HWND (CWnd::m_hWnd @+0x1c). The ex .cpp-local CSliderCtrl view (a CWnd
// + 1 method shell over vtable 0x5ecb24 - the MFC lib one) is dissolved.
// _AFX_ENABLE_INLINES is already #undef'd for the clang label step by
// <Gruntz/Wnd.h> above, so AFXCMN.INL is skipped there like afxwin*.inl.
// ---------------------------------------------------------------------------
#include <afxcmn.h>

// The game-manager settings singleton (g_gameReg), typed as what it IS: CGruntzMgr
// (canonical <Gruntz/GruntzMgr.h>). It used to be typed CGameRegistry* - a second,
// fake header view of this same object - which forced a CGruntzMgr cast at every
// site whose method only the canonical declares, and routed the two
// remaining ones (SetSoundVolume/SetVoiceVolume) to CGameRegistry:: declarations
// that NOTHING defines: ?SetSoundVolume@CGameRegistry@@QAEXH@Z /
// ?SetVoiceVolume@CGameRegistry@@QAEXH@Z were guaranteed unresolved externals, while
// retail's calls there go to ?SetSoundVolume@CGruntzMgr@@QAEXH@Z (0x0919d0) and
// ?SetVoiceVolume@CGruntzMgr@@QAEHH@Z (0x091a10) - which gruntzmgr already defines.
// Typing the pointer correctly binds both calls to the real bodies and the casts fall
// out on their own. Every member this TU touches is on CGruntzMgr or its WAP32::CGameMgr
// base at the same offsets (m_soundEnabled +0x10, m_musicEnabled +0x14, m_savedModeW/H
// +0x94/+0x98, ...), so the swap is byte-neutral.
extern "C" {
}

// The active dialog handle latch (@0x64557c); the proc stamps it on entry.
// DEFINED in Net/LobbyDialogs.cpp (namespace NetLobby); the old `g_curDlg_optdlg`
// alias symbol (nothing defined it) is gone.
#include <Net/NetLobby.h> // NetLobby::g_curDlg

// The CD-prompt result gate (@0x6455ec); DEFINED in StartUpPrompt.cpp (its writer).

// Mode-lock gates (@0x6455b4/bc/c0): when set they grey out option groups AND gate the
// option commits. FABRICATED-SYMBOL FIX (assert_relocs --fake-targets): these were declared
// with C++ linkage, so they mangled to ?g_disableAudio@@3HA etc. - symbols NOTHING defines
// (the storage is the extern-"C" global the rest of the tree already binds), i.e. three
// guaranteed `unresolved external symbol`s. extern "C" makes them the SAME datum. The old
// hex names (g_gate_2455b4/bc/c0, Globals.h + LevelPreview.cpp) were renamed to these
// semantic ones rather than the reverse - best name wins.
// g_disableSound (0x2455bc) / g_disableMusic (0x2455c0) are CONSOLIDATED globals: they come
// from <Globals.h> (included above), inside its extern-"C" block. Re-declaring them here would
// re-proliferate them (the labels gate refuses it). Only 0x2455b4 is not consolidated.
// The [Config] gate band (0x6455b4..0x6455e4) is DEFINED in its owner TU
// src/Rez/RezSync.cpp (RezSync::Init loads all twelve from the .bute [Config] keys
// that name them); the reference externs live in <Globals.h>. DATA() belongs on the
// DEFINITION only - these used to carry a DATA() on a bare `extern`, which binds a
// name to an rva without ever giving it storage.

// The options-dialog staging cells (0x22bd64..0x22bdd4): a snapshot of the live
// settings LoadGameOptionsToDialog captures so IDCANCEL/Apply can restore/commit
// them. Owned by this TU (videoconfig.obj's .bss); DEFINED here, zero-init, with the
// reference externs kept in <Globals.h>. extern "C" (their consolidated linkage).
extern "C" {
    DATA(0x0022bd64)
    i32 g_opt_22bd64 = 0;
    DATA(0x0022bd68)
    i32 g_opt_22bd68 = 0;
    DATA(0x0022bd6c)
    i32 g_opt_22bd6c = 0;
    DATA(0x0022bd70)
    i32 g_opt_22bd70 = 0;
    DATA(0x0022bd84)
    i32 g_opt_22bd84 = 0;
    DATA(0x0022bdc4)
    i32 g_opt_22bdc4 = 0;
    DATA(0x0022bdc8)
    i32 g_opt_22bdc8 = 0;
    DATA(0x0022bdcc)
    i32 g_opt_22bdcc = 0;
    DATA(0x0022bdd0)
    i32 g_opt_22bdd0 = 0;
    DATA(0x0022bdd4)
    i32 g_opt_22bdd4 = 0;
}

// The eight option-control HWNDs the dialog caches at init (GetDlgItem of the
// music / voice / speech / easy / resolution-slider / and three more checkboxes).
// Owned by this TU; DEFINED here (zero-init .bss), DATA()-pinned to their retail rvas.
DATA(0x0022bdd8)
HWND g_optHwndMusic = 0; // IDC 0x46d
DATA(0x0022bddc)
HWND g_optHwndVoice = 0; // IDC 0x475
DATA(0x0022bde0)
HWND g_optHwndSpeech = 0; // IDC 0x471
DATA(0x0022bde4)
HWND g_optHwndEasy = 0; // IDC 0x455
DATA(0x0022bde8)
HWND g_optHwndResSlider = 0; // IDC 0x52c
DATA(0x0022bdec)
HWND g_optHwndCk6 = 0; // IDC 0x472
DATA(0x0022bdf0)
HWND g_optHwndCk7 = 0; // IDC 0x470
DATA(0x0022bdf4)
HWND g_optHwndCk8 = 0; // IDC 0x476
// The scroll-cue throttle globals ScrollDialog's config-cue chain reads.

// Forward decls for the in-TU definitions below (callers precede them in RVA order).
void LoadGameOptionsToDialog(HWND hDlg);                           // 0x036860
void ReadMenuOptionsDialog(HWND hDlg);                             // 0x036a30
void OnToggleMusicOption(HWND hDlg);                               // 0x036d00
void OnToggleVoiceOption(HWND hDlg);                               // 0x036d50
void OnToggleSpeechOption(HWND hDlg);                              // 0x036da0
void OnToggleEasyModeOption(HWND hDlg);                            // 0x036e10
void OnToggleCk5Option(HWND hDlg);                                 // 0x036df0 (thunk 0x19b5;
                                                                   //  unreconstructed extern)
void LoadVideoResolutionConfig(HWND hDlg, i32 nIDCombo, i32 nSel); // 0x036f30
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo, i32 code, i32 pos); // 0x0370a0
void ScrollDialog(HWND hDlg, HWND hCtrl, i32 code, i32 pos);               // 0x037260
void DialogInit37870(HWND hDlg);                                           // 0x037870
void SaveVideoCheckboxes(HWND hDlg);                                       // 0x0378c0
void ApplyGameOptions(); // the dlgproc's free-call shape of ?ApplyGameOptions@CPlay@@ (0x036be0)
namespace ApiCallerStubs {
    void winapi_0371e0_GetDlgItem_SetScrollInfo(HWND hDlg, i32 id, i32 pos, i32 max); // 0x0371e0
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(HWND hDlg, i32 id);                    // 0x036ec0
} // namespace ApiCallerStubs

// CheckDlgButton is the real USER32 import (its IAT slot @0x6c44b4); called twice in
// DialogInit37870, cl caches the __imp__ pointer in a reg once - the exact
// `mov edi,ds:[__imp]; call edi` idiom the ex-`p_CheckDlgButton` fn-ptr global
// hand-modeled. Same treatment as this TU's IsDlgButtonChecked (<Win32.h>/afx).

// 0x363a0: GetResolutionCode - map the live backbuffer (width,height) to the
// resolution combo index (1024x768 -> 3, 800x600 -> 2, else 1).
RVA(0x000363a0, 0x41)
i32 GetResolutionCode() {
    i32 w = g_gameReg->m_savedModeW;
    i32 h = g_gameReg->m_savedModeH;
    if (w == 0x400 && h == 0x300) {
        return RES_1024x768;
    }
    if (w == 0x320 && h == 0x258) {
        return RES_800x600;
    }
    return RES_640x480;
}

// ---------------------------------------------------------------------------
// GameOptionsDlgProc @0x036410, the master game-options dialog procedure.
// WM_INITDIALOG loads the option checkboxes/slider from the settings singleton and
// greys out the ones the current mode locks; WM_COMMAND routes each checkbox toggle
// to its handler and IDOK/IDCANCEL commit + close (re-seeding the saved resolution);
// WM_HSCROLL drives the resolution slider.
//
// @early-stop
// ~92.5%: the full dialog logic + every handler dispatch is byte-exact. Residual is
// pure codegen shaping: (1) the IDOK resolution-store register allocation (retail
// caches g_gameReg once and puts w/h in ecx/edx; cl reloads it and uses eax/ecx),
// (2) IsInPlayState's inline-vs-call bool normalization (GruntzMgr.h defines it inline,
// so cl folds a neg/sbb/neg where retail keeps the call's raw bool test), and (3) the
// outer msg-switch default placement (je-case vs jne-default fall-through). The
// cross-view dispatch (m_curState's game-manager/net dual role via the CNetMgr
// cross-cast) is reloc-masked scaffolding pending those classes' shared modeling.
RVA(0x00036410, 0x366)
BOOL CALLBACK GameOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hDlg;

    switch (msg) {
        case WM_HSCROLL: { // 0x114
            i32 code = static_cast<i32>((wParam & 0xffff));
            i32 pos = static_cast<i32>((wParam >> 0x10));
            if (reinterpret_cast<HWND>(lParam) == g_optHwndResSlider) {
                SaveVideoResolutionConfig(hDlg, reinterpret_cast<HWND>(lParam), code, pos);
            } else {
                ScrollDialog(hDlg, reinterpret_cast<HWND>(lParam), code, pos);
            }
            return TRUE;
        }

        case WM_COMMAND: // 0x111
            switch (wParam) {
                case 2: // IDCANCEL: discard
                    if (g_gameReg->m_curState->Update() == 0x11) {
                        (static_cast<CMulti*>(g_gameReg->m_curState))->SendChannelStat423();
                    }
                    ApplyGameOptions();
                    EndDialog(hDlg, 0);
                    return TRUE;
                case 1: { // IDOK: commit
                    if (g_gameReg->m_curState->Update() == 0x11) {
                        (static_cast<CMulti*>(g_gameReg->m_curState))->SendChannelStat423();
                    }
                    ReadMenuOptionsDialog(hDlg);
                    EndDialog(hDlg, 1);
                    i32 w, h;
                    if (g_videoResolutionMode == 3) {
                        w = 0x400;
                        h = 0x300;
                    } else if (g_videoResolutionMode == 2) {
                        w = 0x320;
                        h = 0x258;
                    } else {
                        w = 0x280;
                        h = 0x1e0;
                    }
                    g_gameReg->m_savedModeW = w;
                    g_gameReg->m_savedModeH = h;
                    if (g_gameReg->IsInPlayState()) {
                        g_gameReg->CheckSavedMode();
                    }
                    return TRUE;
                }
            }
            // control notifications: route each checkbox to its handler
            if (g_optHwndMusic != 0 && reinterpret_cast<HWND>(lParam) == g_optHwndMusic) {
                OnToggleMusicOption(hDlg);
                return FALSE;
            }
            if (g_optHwndVoice != 0 && reinterpret_cast<HWND>(lParam) == g_optHwndVoice) {
                OnToggleVoiceOption(hDlg);
                return FALSE;
            }
            if (g_optHwndSpeech != 0 && reinterpret_cast<HWND>(lParam) == g_optHwndSpeech) {
                OnToggleSpeechOption(hDlg);
                return FALSE;
            }
            if (g_optHwndEasy != 0 && reinterpret_cast<HWND>(lParam) == g_optHwndEasy) {
                OnToggleEasyModeOption(hDlg);
                return FALSE;
            }
            if (g_optHwndResSlider != 0 && reinterpret_cast<HWND>(lParam) == g_optHwndResSlider) {
                OnToggleCk5Option(hDlg);
                return FALSE;
            }
            return FALSE;

        case WM_INITDIALOG: { // 0x110
            LoadGameOptionsToDialog(hDlg);
            g_optHwndMusic = GetDlgItem(hDlg, 0x46d);
            g_optHwndVoice = GetDlgItem(hDlg, 0x475);
            g_optHwndSpeech = GetDlgItem(hDlg, 0x471);
            g_optHwndEasy = GetDlgItem(hDlg, 0x455);
            g_optHwndResSlider = GetDlgItem(hDlg, 0x52c);
            g_optHwndCk6 = GetDlgItem(hDlg, 0x472);
            g_optHwndCk7 = GetDlgItem(hDlg, 0x470);
            g_optHwndCk8 = GetDlgItem(hDlg, 0x476);

            if (g_gameReg->m_curState->Update() != 3) {
                if (g_gameReg->m_curState->Update() == 0x11) {
                    (static_cast<CMulti*>(g_gameReg->m_curState))->SendChannelStat422();
                } else {
                    EnableWindow(g_optHwndEasy, g_cdPromptResult == 0);
                }
            }
            if (g_disableAudio) {
                EnableWindow(g_optHwndMusic, 0);
                EnableWindow(g_optHwndCk7, 0);
                EnableWindow(g_optHwndVoice, 0);
                EnableWindow(g_optHwndCk8, 0);
                EnableWindow(g_optHwndSpeech, 0);
                EnableWindow(g_optHwndCk6, 0);
            }
            if (g_disableSound) {
                EnableWindow(g_optHwndMusic, 0);
                EnableWindow(g_optHwndCk7, 0);
                EnableWindow(g_optHwndVoice, 0);
                EnableWindow(g_optHwndCk8, 0);
            }
            if (g_disableMusic != 0 || g_gameReg->m_sound->m_enabled == 0) {
                EnableWindow(g_optHwndSpeech, 0);
                EnableWindow(g_optHwndCk6, 0);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// LoadGameOptionsToDialog @0x036860 - the inverse of ReadMenuOptionsDialog: snapshot
// the live settings into the g_opt_* staging globals (so a later Cancel can
// restore / Apply can commit them), then push each value into the options dialog's
// checkboxes (CheckDlgButton via the cached IAT slot 0x6c44b4) and sliders
// (winapi_0371e0). The video-resolution combo + caption go through
// LoadVideoResolutionConfig. A free __cdecl function (no `this`); bails if no manager.
RVA(0x00036860, 0x16f)
void LoadGameOptionsToDialog(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    g_opt_22bd70 = g_gameReg->m_isEasyMode;
    g_opt_22bd6c = g_gameReg->m_soundVolume;
    g_opt_22bd84 = g_gameReg->m_soundEnabled;
    g_opt_22bdc4 = g_gameReg->m_voiceVolume;
    g_opt_22bdd4 = g_gameReg->m_isVoiceEnabled;
    g_opt_22bdcc = g_gameReg->m_sound->GetXMidiVolume();
    g_opt_22bdd0 = g_gameReg->m_musicEnabled;
    g_opt_22bd68 = g_gameReg->m_scrollSpeed;
    g_opt_22bd64 = g_gameReg->m_musicEnabled;
    g_opt_22bdc8 = GetResolutionCode();
    g_videoResolutionMode = GetResolutionCode();

    CheckDlgButton(hDlg, 0x455, g_gameReg->m_isEasyMode);
    LoadVideoResolutionConfig(hDlg, 0x52c, g_videoResolutionMode);
    CheckDlgButton(hDlg, 0x46d, g_gameReg->m_soundEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x470,
        g_gameReg->m_soundVolume,
        0x50
    );
    CheckDlgButton(hDlg, 0x475, g_gameReg->m_isVoiceEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x476,
        g_gameReg->m_voiceVolume,
        0x50
    );
    CheckDlgButton(hDlg, 0x471, g_gameReg->m_musicEnabled);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x472,
        g_gameReg->m_sound->GetXMidiVolume(),
        0x64
    );
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x478,
        g_gameReg->m_scrollSpeed,
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
    if (g_gameReg == 0) {
        return;
    }
    g_gameReg->m_isEasyMode = IsDlgButtonChecked(hDlg, 0x455);
    i32 res = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x52c);
    if (res >= 0 && res <= 100) {
        g_videoResolutionMode = res;
    }
    if (g_disableAudio == 0) {
        if (g_disableSound == 0) {
            g_gameReg->SetRunState(IsDlgButtonChecked(hDlg, 0x46d));
            i32 mv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x470);
            if (mv >= 0 && mv <= 100) {
                g_gameReg->SetSoundVolume(mv);
            }
            g_gameReg->m_isVoiceEnabled = IsDlgButtonChecked(hDlg, 0x475);
            i32 sv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x476);
            if (sv >= 0 && sv <= 100) {
                g_gameReg->SetVoiceVolume(sv);
            }
        }
        if (g_disableAudio == 0 && g_disableMusic == 0 && g_gameReg->m_sound->m_enabled != 0) {
            g_gameReg->SetSoundLevelState(IsDlgButtonChecked(hDlg, 0x471));
            i32 pv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x472);
            if (pv >= 0 && pv <= 100) {
                g_gameReg->m_sound->SetXMidiVolume(pv);
            }
        }
    }
    i32 qv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x478);
    if (qv >= 0 && qv <= 100) {
        g_gameReg->m_scrollSpeed = qv;
    }
}

// ===========================================================================
// CPlay::ApplyGameOptions (0x036be0, dossier seam: play -> this TU) - push the
// current staged option values into the game manager (*g_gameReg). Mirrors the
// video-resolution mode global, stamps the easy/voice/scroll manager words, and -
// unless the runtime lock gates say otherwise - forwards the input flag/state
// options and (when the sound object's m_enabled gate is live) commits the music
// state + XMIDI volume. The staged-value RESTORE twin of ReadMenuOptionsDialog
// (the dlgproc's IDCANCEL path calls it to roll the live settings back).
// ===========================================================================
// @early-stop
// register-coloring wall (~83%). Control flow, all member offsets
// (+0x118/+0x100/+0x124/+0x48/+0x28), the 12 option/gate globals, the 5 callees
// and the redundant lock-gate re-test are byte-faithful and all relocs pair;
// the residual is the non-steerable eax-vs-ecx coloring of the reloaded manager
// pointer (retail pins it in ecx, our cl picks eax) which cascades into the temp
// regs + the top videomode/lock-load schedule. See docs/patterns/zero-register-pinning.md.
RVA(0x00036be0, 0xd3)
void CPlay::ApplyGameOptions() {
    if (g_gameReg == 0) {
        return;
    }
    g_gameReg->m_isEasyMode = g_opt_22bd70;
    g_videoResolutionMode = g_opt_22bdc8;
    if (g_disableAudio == 0) {
        if (g_disableSound == 0) {
            g_gameReg->SetRunState(g_opt_22bd84);
            g_gameReg->SetSoundVolume(g_opt_22bd6c);
            g_gameReg->m_isVoiceEnabled = g_opt_22bdd4;
            g_gameReg->SetVoiceVolume(g_opt_22bdc4);
        }
        if (g_disableAudio == 0 && g_disableMusic == 0 && g_gameReg->m_sound->m_enabled != 0) {
            g_gameReg->SetSoundLevelState(g_opt_22bdd0);
            g_gameReg->m_sound->SetXMidiVolume(g_opt_22bdcc);
        }
    }
    g_gameReg->m_scrollSpeed = g_opt_22bd68;
}

// The four per-checkbox WM_COMMAND handlers of the same options dialog: each
// mirrors one checkbox into the CGruntzMgr settings singleton and enables the
// paired slider/control. Free __cdecl(hWnd); no-op when the manager is not live.
// SAME control IDs + commit set as ReadMenuOptionsDialog above (the reader's
// per-control split-out). g_gameReg loads reloc-mask; the commit calls
// (SetRunState/SetSoundLevelState) go through the thunks 0x492340/0x4923b0.

// 0x36d00: music-enable checkbox (0x46d) -> SetRunState, enable slider 0x470.
RVA(0x00036d00, 0x40)
void OnToggleMusicOption(HWND hWnd) {
    if (g_gameReg) {
        i32 state = IsDlgButtonChecked(hWnd, 0x46d);
        g_gameReg->SetRunState(state);
        EnableWindow(GetDlgItem(hWnd, 0x470), state);
    }
}

// 0x36d50: voice-enable checkbox (0x475) -> m_isVoiceEnabled, enable ctrl 0x476.
RVA(0x00036d50, 0x3c)
void OnToggleVoiceOption(HWND hWnd) {
    if (g_gameReg) {
        i32 checked = IsDlgButtonChecked(hWnd, 0x475);
        g_gameReg->m_isVoiceEnabled = checked;
        EnableWindow(GetDlgItem(hWnd, 0x476), checked);
    }
}

// 0x36da0: speech-enable checkbox (0x471) -> SetSoundLevelState, enable ctrl 0x472.
RVA(0x00036da0, 0x40)
void OnToggleSpeechOption(HWND hWnd) {
    if (g_gameReg) {
        i32 state = IsDlgButtonChecked(hWnd, 0x471);
        g_gameReg->SetSoundLevelState(state);
        EnableWindow(GetDlgItem(hWnd, 0x472), state);
    }
}

// 0x36e10: easy-mode checkbox (0x455) -> m_isEasyMode.
RVA(0x00036e10, 0x26)
void OnToggleEasyModeOption(HWND hWnd) {
    if (g_gameReg) {
        g_gameReg->m_isEasyMode = IsDlgButtonChecked(hWnd, 0x455);
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
} // namespace ApiCallerStubs

// ---------------------------------------------------------------------------
// LoadVideoResolutionConfig (0x036f30)
// hDlg            - the owning dialog.
// nIDCombo        - control ID of the resolution combo (resolved via GetDlgItem).
// nSel            - the selection index to push into the combo / option control.
// Resolves the engine option-control wrapper, seeds its range (1,3,1), forwards
// nSel to the wrapped child (msg 0x405), then rebuilds the "Video Resolution
// (WxH)" caption on the IDC_RESCAPTION static text from the global mode.
//
// BYTE-EXACT body modulo one MSVC5 deferred-callee-save (`push ebp`) scheduling
// coin-flip (the target defers the ebp save past the four early-return guards;
// our cl saves it eagerly).
RVA(0x00036f30, 0x114)
void LoadVideoResolutionConfig(HWND hDlg, i32 nIDCombo, i32 nSel) {
    if (!hDlg) {
        return;
    }

    HWND hCombo = GetDlgItem(hDlg, nIDCombo);
    if (!hCombo) {
        return;
    }

    CSliderCtrl* pCtrl = static_cast<CSliderCtrl*>(CWnd::FromHandle(hCombo));
    if (!pCtrl) {
        return;
    }

    pCtrl->SetRange(1, 3, 1);
    SendMessageA(pCtrl->m_hWnd, 0x405, 1, nSel);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption) {
        return;
    }

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
        case RES_640x480:
            strcat(szCaption, "(640x480)");
            break;
        case RES_800x600:
            strcat(szCaption, "(800x600)");
            break;
        case RES_1024x768:
            strcat(szCaption, "(1024x768)");
            break;
        default:
            return;
    }
    SetWindowTextA(hCaption, szCaption);
}

// ---------------------------------------------------------------------------
// SaveVideoResolutionConfig (0x0370a0)
// hDlg     - the owning dialog (for IDC_RESCAPTION).
// hCombo   - the resolution combo HWND. (code/pos are the WM_HSCROLL params the
//            dlgproc forwards; unused by the body - the combo is re-queried.)
// Reads the combo's current selection (engine msg 0x400 -> the wrapped child),
// stores it into the global mode, then rebuilds the caption (same tail as Load).
RVA(0x000370a0, 0xf1)
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo, i32 /*code*/, i32 /*pos*/) {
    CWnd* pCtrl = CWnd::FromHandle(static_cast<HWND__*>(hCombo));
    if (!pCtrl) {
        return;
    }

    g_videoResolutionMode = SendMessageA(pCtrl->m_hWnd, 0x400, 0, 0);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption) {
        return;
    }

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
        case RES_640x480:
            strcat(szCaption, "(640x480)");
            break;
        case RES_800x600:
            strcat(szCaption, "(800x600)");
            break;
        case RES_1024x768:
            strcat(szCaption, "(1024x768)");
            break;
        default:
            return;
    }
    SetWindowTextA(hCaption, szCaption);
}

namespace ApiCallerStubs {
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
// ScrollDialog (0x037260) - the options-dialog slider handler. Reads the control's
// SCROLLINFO, adjusts nPos by the SB_* code, writes it back, then routes the new
// value to the matching setting: 0x472/0x478 store directly (XMidi volume / scroll
// speed); 0x476/0x470 store the flag AND (re)trigger a GAME_VOICE / GAME_CHIPFALLOUT
// config cue if the kill-cue clock has elapsed. A free __cdecl(hDlg, hCtrl, code, pos)
// helper GameOptionsDlgProc's WM_HSCROLL dispatches to.
//
// The config-cue chain matches PathHazard's: the CSndHost at m_world->m_soundRegistry gates on
// m_emitGate, CSndFinder::Lookup resolves the named LeafCue, the g_sndEnabled/kill-
// clock cooldown gate throttles, then LeafCue::m_10->ConfigureItem plays it.
//
// @early-stop
// regalloc wall + jump-table-data artifact (docs/patterns/jumptable-data-overlap.md).
// Logic + instruction selection identical, but MSVC5 caches `code` in ebp and `newpos`
// in edi across the whole body, whereas retail keeps `newpos` in ebp, holds `code` in
// eax only for the switch, and RE-READS code from the stack in the voice/chip blocks;
// that register permutation shifts most operand bytes (consistent ebp<->edi/eax swap,
// llvm-objdump -dr). ~85%.
RVA(0x00037260, 0x1fd)
void ScrollDialog(HWND hDlg, HWND hCtrl, i32 code, i32 pos) {
    if (!hCtrl) {
        return;
    }
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    GetScrollInfo(hCtrl, SB_CTL, &si);
    i32 newpos;
    if (code == 5) {
        newpos = pos;
    } else {
        newpos = si.nPos;
        if (code == 4) {
            newpos = pos;
        }
    }
    switch (code) {
        case 0:
            newpos--;
            break;
        case 1:
            newpos++;
            break;
        case 2:
            newpos -= 10;
            break;
        case 3:
            newpos += 10;
            break;
        case 4:
            break;
        case 5:
            break;
        default:
            return;
    }
    si.fMask = SIF_POS;
    si.nPos = newpos;
    SetScrollInfo(hCtrl, SB_CTL, &si, TRUE);
    if (hCtrl == GetDlgItem(hDlg, 0x472)) {
        g_gameReg->m_sound->SetXMidiVolume(newpos);
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x478)) {
        g_gameReg->m_scrollSpeed = newpos;
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x476)) {
        g_gameReg->SetVoiceVolume(newpos);
        if (code == 5) {
            return;
        }
        CSndHost* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate) {
            return;
        }
        void* cue_ob = 0;
        host->m_10.Lookup("GAME_VOICE", cue_ob);
        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if (static_cast<u32>((g_killCueClock - cue->m_14)) < static_cast<u32>(cue->m_18)) {
            return;
        }
        cue->m_14 = g_killCueClock;
        cue->m_10->ConfigureItem(newpos, 0, 0, 0);
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x470)) {
        g_gameReg->SetSoundVolume(newpos);
        if (code == 5) {
            return;
        }
        CSndHost* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate) {
            return;
        }
        void* cue_ob = 0;
        host->m_10.Lookup("GAME_CHIPFALLOUT", cue_ob);
        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if (static_cast<u32>((g_killCueClock - cue->m_14)) < static_cast<u32>(cue->m_18)) {
            return;
        }
        cue->m_14 = g_killCueClock;
        cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
        return;
    }
}

// 0x377e0: VideoOptionsDlgProc - the video-options dialog procedure. WM_INITDIALOG
// seeds the checkboxes (DialogInit37870); WM_COMMAND/IDOK latches them
// (SaveVideoCheckboxes) and closes with 1, IDCANCEL closes with 0.
RVA(0x000377e0, 0x6a)
BOOL CALLBACK VideoOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            DialogInit37870(hDlg);
            return TRUE;
        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    SaveVideoCheckboxes(hDlg);
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

// 0x37870 (re-homed from src/Stub/BoundaryMisc.cpp): DialogInit37870 - seed the two
// video option checkboxes (IDC 0x46f / 0x4d5) from the settings singleton's
// m_isHighDetail / m_isEffectsEnabled flags, via the cached CheckDlgButton import.
// @early-stop
// 93.33% - regalloc wall: cl pins hDlg in edi and the cached import ptr in esi;
// retail swaps them (ptr in edi, hDlg in esi). The cached-pointer shape, both
// CheckDlgButton calls, the arg tuples and the null guard are byte-exact; the
// edi/esi assignment is not source-steerable.
RVA(0x00037870, 0x3c)
void DialogInit37870(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    CheckDlgButton(hDlg, 0x46f, g_gameReg->m_isHighDetail);
    CheckDlgButton(hDlg, 0x4d5, g_gameReg->m_isEffectsEnabled);
}

// 0x378c0: SaveVideoCheckboxes(hDlg) - latch the two video option checkboxes
// (IDC 0x46f smooth-scroll, 0x4d5 show-fps) into the settings singleton. No-op
// when the singleton is not yet live.
// @early-stop
// 99.5%: the same deferred-callee-save (push) scheduling coin-flip as this TU's
// resolution-config pair; logic + offsets byte-exact.
RVA(0x000378c0, 0x40)
void SaveVideoCheckboxes(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    g_gameReg->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_gameReg->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
SIZE_UNKNOWN(CSliderCtrl);
