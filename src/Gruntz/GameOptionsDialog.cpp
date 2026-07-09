#include <Mfc.h> // afx.h FIRST (GameRegistry.h pulls MFC)
#include <rva.h>
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_mgrSettings singleton)
#include <Gruntz/GruntzMgr.h>    // CGruntzMgr (IsInPlayState / CheckSavedMode)
#include <Gruntz/State.h>        // CState::Update (m_curState state probe)
#include <Net/NetMgr.h>          // CNetMgr::SendChannelStat422/423 (dispatched on m_curState)
#include <Win32.h>               // GetDlgItem / EnableWindow / EndDialog / HWND

// GameOptionsDialog.cpp - GameOptionsDlgProc @0x036410, the master game-options
// dialog procedure (C:\Proj\Gruntz). WM_INITDIALOG loads the option checkboxes/
// slider from the settings singleton and greys out the ones the current mode locks;
// WM_COMMAND routes each checkbox toggle to its handler and IDOK/IDCANCEL commit +
// close (re-seeding the saved resolution); WM_HSCROLL drives the resolution slider.
//
// @early-stop
// ~92.5%: the full dialog logic + every handler dispatch is byte-exact. Residual is
// pure codegen shaping: (1) the IDOK resolution-store register allocation (retail
// caches g_mgrSettings once and puts w/h in ecx/edx; cl reloads it and uses eax/ecx),
// (2) IsInPlayState's inline-vs-call bool normalization (GruntzMgr.h defines it inline,
// so cl folds a neg/sbb/neg where retail keeps the call's raw bool test), and (3) the
// outer msg-switch default placement (je-case vs jne-default fall-through). The
// cross-view dispatch (m_curState's game-manager/net dual role via CGruntzMgr/CNetMgr
// cross-casts, +0x48 sound gate by raw read) is reloc-masked scaffolding pending those
// classes' shared modeling. Homed off src/Stub; final sweep re-attacks the regalloc.

// The settings singleton (?g_mgrSettings@@... == g_gameReg @0x64556c); DATA-bound in
// VideoConfig.cpp, extern here.
extern CGameRegistry* g_mgrSettings;

// The active dialog handle latch (NetLobby::g_curDlg_64557c @0x64557c); the proc
// stamps it on entry. DATA-bound in Net/LobbyDialogs.cpp; extern here.
extern HWND g_curDlg_optdlg; // aliases 0x64557c (reloc-masked)

// The video-resolution mode discriminator (@0x60ccc4; 1=640x480/2=800x600/3=1024x768).
// DATA-bound in VideoConfig.cpp.
extern i32 g_videoResolutionMode;

// The CD-prompt result gate (?g_6455ec@@3HA @0x6455ec); DATA-bound in GruntzMgrCmd.cpp.
extern i32 g_cdPromptResult;

// Mode-lock gates (@0x6455b4/bc/c0): when set they grey out option groups.
DATA(0x002455b4)
extern i32 g_optLockAll;
DATA(0x002455bc)
extern i32 g_optLockAudio;
DATA(0x002455c0)
extern i32 g_optLockSpeech;

// The eight option-control HWNDs the dialog caches at init (GetDlgItem of the
// music / voice / speech / easy / resolution-slider / and three more checkboxes).
DATA(0x0022bdd8)
extern HWND g_optHwndMusic; // IDC 0x46d
DATA(0x0022bddc)
extern HWND g_optHwndVoice; // IDC 0x475
DATA(0x0022bde0)
extern HWND g_optHwndSpeech; // IDC 0x471
DATA(0x0022bde4)
extern HWND g_optHwndEasy; // IDC 0x455
DATA(0x0022bde8)
extern HWND g_optHwndResSlider; // IDC 0x52c
DATA(0x0022bdec)
extern HWND g_optHwndCk6; // IDC 0x472
DATA(0x0022bdf0)
extern HWND g_optHwndCk7; // IDC 0x470
DATA(0x0022bdf4)
extern HWND g_optHwndCk8; // IDC 0x476

// Option-handler free functions (MenuState.cpp / Play.cpp / VideoConfig.cpp /
// ApiWrappers), referenced by their real symbols (reloc-masked, no body here).
void LoadGameOptionsToDialog(HWND hDlg); // 0x036860
void ReadMenuOptionsDialog(HWND hDlg);   // 0x036a30
void OnToggleMusicOption(HWND hDlg);     // 0x036d00
void OnToggleVoiceOption(HWND hDlg);     // 0x036d50
void OnToggleSpeechOption(HWND hDlg);    // 0x036da0
void OnToggleEasyModeOption(HWND hDlg);  // 0x036e10
void OnToggleCk5Option(HWND hDlg);       // 0x036df0 (thunk 0x19b5)
void SaveVideoResolutionConfig(HWND hDlg, HWND hCtrl, i32 code, i32 pos); // 0x0370a0
void ScrollDialog(HWND hDlg, HWND hCtrl, i32 code, i32 pos);              // 0x037260

void ApplyGameOptions(); // ?ApplyGameOptions@CPlay@@ 0x036be0 (dispatched free here)

// ---------------------------------------------------------------------------
RVA(0x00036410, 0x366)
BOOL CALLBACK GameOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    g_curDlg_optdlg = hDlg;

    switch (msg) {
        case WM_HSCROLL: { // 0x114
            i32 code = (i32)(wParam & 0xffff);
            i32 pos = (i32)(wParam >> 0x10);
            if ((HWND)lParam == g_optHwndResSlider) {
                SaveVideoResolutionConfig(hDlg, (HWND)lParam, code, pos);
            } else {
                ScrollDialog(hDlg, (HWND)lParam, code, pos);
            }
            return TRUE;
        }

        case WM_COMMAND: // 0x111
            switch (wParam) {
                case 2: // IDCANCEL: discard
                    if (g_mgrSettings->m_curState->Update() == 0x11) {
                        ((CNetMgr*)g_mgrSettings->m_curState)->SendChannelStat423();
                    }
                    ApplyGameOptions();
                    EndDialog(hDlg, 0);
                    return TRUE;
                case 1: { // IDOK: commit
                    if (g_mgrSettings->m_curState->Update() == 0x11) {
                        ((CNetMgr*)g_mgrSettings->m_curState)->SendChannelStat423();
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
                    g_mgrSettings->m_savedModeW = w;
                    g_mgrSettings->m_savedModeH = h;
                    if (((CGruntzMgr*)g_mgrSettings)->IsInPlayState()) {
                        ((CGruntzMgr*)g_mgrSettings)->CheckSavedMode();
                    }
                    return TRUE;
                }
            }
            // control notifications: route each checkbox to its handler
            if (g_optHwndMusic != 0 && (HWND)lParam == g_optHwndMusic) {
                OnToggleMusicOption(hDlg);
                return FALSE;
            }
            if (g_optHwndVoice != 0 && (HWND)lParam == g_optHwndVoice) {
                OnToggleVoiceOption(hDlg);
                return FALSE;
            }
            if (g_optHwndSpeech != 0 && (HWND)lParam == g_optHwndSpeech) {
                OnToggleSpeechOption(hDlg);
                return FALSE;
            }
            if (g_optHwndEasy != 0 && (HWND)lParam == g_optHwndEasy) {
                OnToggleEasyModeOption(hDlg);
                return FALSE;
            }
            if (g_optHwndResSlider != 0 && (HWND)lParam == g_optHwndResSlider) {
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

            if (g_mgrSettings->m_curState->Update() != 3) {
                if (g_mgrSettings->m_curState->Update() == 0x11) {
                    ((CNetMgr*)g_mgrSettings->m_curState)->SendChannelStat422();
                } else {
                    EnableWindow(g_optHwndEasy, g_cdPromptResult == 0);
                }
            }
            if (g_optLockAll) {
                EnableWindow(g_optHwndMusic, 0);
                EnableWindow(g_optHwndCk7, 0);
                EnableWindow(g_optHwndVoice, 0);
                EnableWindow(g_optHwndCk8, 0);
                EnableWindow(g_optHwndSpeech, 0);
                EnableWindow(g_optHwndCk6, 0);
            }
            if (g_optLockAudio) {
                EnableWindow(g_optHwndMusic, 0);
                EnableWindow(g_optHwndCk7, 0);
                EnableWindow(g_optHwndVoice, 0);
                EnableWindow(g_optHwndCk8, 0);
            }
            if (g_optLockSpeech != 0 || *(i32*)((char*)g_mgrSettings->m_sound + 0x28) == 0) {
                EnableWindow(g_optHwndSpeech, 0);
                EnableWindow(g_optHwndCk6, 0);
            }
            return TRUE;
        }
    }
    return FALSE;
}
