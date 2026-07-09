#include <Mfc.h> // afx.h FIRST (GameRegistry.h pulls MFC)
#include <rva.h>
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_mgrSettings singleton)
#include <Gruntz/GruntzMgr.h>    // CGruntzMgr (IsInPlayState / CheckSavedMode)
#include <Gruntz/State.h>        // CState::Update (m_curState state probe)
#include <Gruntz/LeafCue.h>      // LeafCue (the config-cue leaf: m_10/m_14/m_18) for ScrollDialog
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
void LoadGameOptionsToDialog(HWND hDlg);                                  // 0x036860
void ReadMenuOptionsDialog(HWND hDlg);                                    // 0x036a30
void OnToggleMusicOption(HWND hDlg);                                      // 0x036d00
void OnToggleVoiceOption(HWND hDlg);                                      // 0x036d50
void OnToggleSpeechOption(HWND hDlg);                                     // 0x036da0
void OnToggleEasyModeOption(HWND hDlg);                                   // 0x036e10
void OnToggleCk5Option(HWND hDlg);                                        // 0x036df0 (thunk 0x19b5)
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

// ---------------------------------------------------------------------------
// ScrollDialog (0x037260) - the options-dialog slider handler. Reads the control's
// SCROLLINFO, adjusts nPos by the SB_* code, writes it back, then routes the new
// value to the matching setting: 0x472/0x478 store directly (XMidi volume / scroll
// speed); 0x476/0x470 store the flag AND (re)trigger a GAME_VOICE / GAME_CHIPFALLOUT
// config cue if the kill-cue clock has elapsed. A free __cdecl(hDlg, hCtrl, code, pos)
// helper GameOptionsDlgProc's WM_HSCROLL dispatches to.
//
// The config-cue chain matches PathHazard's: the CSndHost at m_world->m_28 gates on
// m_emitGate, CSndFinder::Lookup resolves the named LeafCue, the g_sndEnabled/kill-
// clock cooldown gate throttles, then LeafCue::m_10->ConfigureItem plays it.

extern i32 g_sndEnabled;       // 0x61ab20 (?g_sndEnabled@@3HA)
extern i32 g_sndCueTag;        // 0x61ab24 (?g_sndCueTag@@3HA)
extern "C" i32 g_killCueClock; // 0x6bf3c0 (_g_killCueClock)

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
        ((CGruntzSoundZ*)g_mgrSettings->m_sound)->SetXMidiVolume(newpos);
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x478)) {
        g_mgrSettings->m_scrollSpeed = newpos;
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x476)) {
        ((CGruntzMgr*)g_mgrSettings)->StoreInputState(newpos);
        if (code == 5) {
            return;
        }
        CSndHost* host = g_mgrSettings->m_world->m_28;
        if (host->m_emitGate) {
            return;
        }
        LeafCue* cue = 0;
        host->m_10.Lookup("GAME_VOICE", &cue);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if ((u32)(g_killCueClock - cue->m_14) < (u32)cue->m_18) {
            return;
        }
        cue->m_14 = g_killCueClock;
        cue->m_10->ConfigureItem(newpos, 0, 0, 0);
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x470)) {
        ((CGruntzMgr*)g_mgrSettings)->StoreInputFlag(newpos);
        if (code == 5) {
            return;
        }
        CSndHost* host = g_mgrSettings->m_world->m_28;
        if (host->m_emitGate) {
            return;
        }
        LeafCue* cue = 0;
        host->m_10.Lookup("GAME_CHIPFALLOUT", &cue);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if ((u32)(g_killCueClock - cue->m_14) < (u32)cue->m_18) {
            return;
        }
        cue->m_14 = g_killCueClock;
        cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
        return;
    }
}
