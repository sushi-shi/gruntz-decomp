#include <rva.h>

#include <Gruntz/VideoConfig.h>

#include <Mfc.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Play.h>
#include <Gruntz/Resolution.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/State.h>
#include <Gruntz/Wnd.h>
#include <MsgParam.h>
#include <Net/NetLobby.h>
#include <Net/NetMgr.h>
#include <Rez/RezSync.h>

#include <afxcmn.h>
#include <string.h>

typedef enum VideoConfigDlgId {
    IDC_RESCAPTION = 0x52d,
} VideoConfigDlgId;

DATA(0x0020ccc4)
Resolution g_videoResolutionMode = RES_640x480;

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
Resolution g_opt_22bdc8 = RES_UNSET;
DATA(0x0022bdcc)
i32 g_opt_22bdcc = 0;
DATA(0x0022bdd0)
i32 g_opt_22bdd0 = 0;
DATA(0x0022bdd4)
i32 g_opt_22bdd4 = 0;

DATA(0x0022bdd8)
HWND g_optHwndMusic = 0;
DATA(0x0022bddc)
HWND g_optHwndVoice = 0;
DATA(0x0022bde0)
HWND g_optHwndSpeech = 0;
DATA(0x0022bde4)
HWND g_optHwndEasy = 0;
DATA(0x0022bde8)
HWND g_optHwndResSlider = 0;
DATA(0x0022bdec)
HWND g_optHwndCk6 = 0;
DATA(0x0022bdf0)
HWND g_optHwndCk7 = 0;
DATA(0x0022bdf4)
HWND g_optHwndCk8 = 0;

RVA(0x000363a0, 0x41)
Resolution GetResolutionCode() {
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

RVA(0x00036410, 0x366)
BOOL CALLBACK GameOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hDlg;

    switch (msg) {
        case WM_HSCROLL: {
            i32 code = static_cast<i32>((wParam & 0xffff));
            i32 pos = static_cast<i32>((wParam >> 0x10));

            MsgParam from;
            from.m_lparam = lParam;
            HWND bar = from.m_hwnd;
            if (bar == g_optHwndResSlider) {
                SaveVideoResolutionConfig(hDlg, bar, code, pos);
            } else {
                ScrollDialog(hDlg, bar, code, pos);
            }
            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam) {
                case 2:
                    if (g_gameReg->m_curState->Update() == GAMESTATE_MULTI) {
                        (static_cast<CMulti*>(g_gameReg->m_curState))->SendChannelStat423();
                    }
                    ApplyGameOptions();
                    EndDialog(hDlg, 0);
                    return TRUE;
                case 1: {
                    if (g_gameReg->m_curState->Update() == GAMESTATE_MULTI) {
                        (static_cast<CMulti*>(g_gameReg->m_curState))->SendChannelStat423();
                    }
                    ReadMenuOptionsDialog(hDlg);
                    EndDialog(hDlg, 1);
                    i32 w, h;
                    if (g_videoResolutionMode == RES_1024x768) {
                        w = 0x400;
                        h = 0x300;
                    } else if (g_videoResolutionMode == RES_800x600) {
                        w = 0x320;
                        h = 0x258;
                    } else {
                        w = 0x280;
                        h = 0x1e0;
                    }
                    CGruntzMgr* reg = g_gameReg;
                    reg->m_savedModeW = w;
                    reg->m_savedModeH = h;
                    if (g_gameReg->IsInPlayState()) {
                        g_gameReg->CheckSavedMode();
                    }
                    return TRUE;
                }
            }

            {
                MsgParam from;
                from.m_lparam = lParam;
                HWND ctrl = from.m_hwnd;
                if (g_optHwndMusic != 0 && ctrl == g_optHwndMusic) {
                    OnToggleMusicOption(hDlg);
                    return FALSE;
                }
                if (g_optHwndVoice != 0 && ctrl == g_optHwndVoice) {
                    OnToggleVoiceOption(hDlg);
                    return FALSE;
                }
                if (g_optHwndSpeech != 0 && ctrl == g_optHwndSpeech) {
                    OnToggleSpeechOption(hDlg);
                    return FALSE;
                }
                if (g_optHwndEasy != 0 && ctrl == g_optHwndEasy) {
                    OnToggleEasyModeOption(hDlg);
                    return FALSE;
                }

                if (g_optHwndResSlider != 0 && ctrl == g_optHwndResSlider) {
                    OnToggleCk5Option(hDlg);
                }
            }

            break;

        case WM_INITDIALOG: {
            LoadGameOptionsToDialog(hDlg);
            g_optHwndMusic = GetDlgItem(hDlg, 0x46d);
            g_optHwndVoice = GetDlgItem(hDlg, 0x475);
            g_optHwndSpeech = GetDlgItem(hDlg, 0x471);
            g_optHwndEasy = GetDlgItem(hDlg, 0x455);
            g_optHwndResSlider = GetDlgItem(hDlg, 0x52c);
            g_optHwndCk6 = GetDlgItem(hDlg, 0x472);
            g_optHwndCk7 = GetDlgItem(hDlg, 0x470);
            g_optHwndCk8 = GetDlgItem(hDlg, 0x476);

            if (g_gameReg->m_curState->Update() != GAMESTATE_PLAY) {
                if (g_gameReg->m_curState->Update() == GAMESTATE_MULTI) {
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
    ConfigureDialogScrollBar(hDlg, 0x470, g_gameReg->m_soundVolume, 0x50);
    CheckDlgButton(hDlg, 0x475, g_gameReg->m_isVoiceEnabled);
    ConfigureDialogScrollBar(hDlg, 0x476, g_gameReg->m_voiceVolume, 0x50);
    CheckDlgButton(hDlg, 0x471, g_gameReg->m_musicEnabled);
    ConfigureDialogScrollBar(hDlg, 0x472, g_gameReg->m_sound->GetXMidiVolume(), 0x64);
    ConfigureDialogScrollBar(hDlg, 0x478, g_gameReg->m_scrollSpeed, 0x64);
}

// @early-stop
RVA(0x00036a30, 0x14e)
void ReadMenuOptionsDialog(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    g_gameReg->m_isEasyMode = IsDlgButtonChecked(hDlg, 0x455);
    // The slider position IS the mode index the dialog offers.
    Resolution res = static_cast<Resolution>(GetDialogScrollPosition(hDlg, 0x52c));
    if (res >= RES_UNSET && res <= 100) {
        g_videoResolutionMode = res;
    }
    if (g_disableAudio == 0) {
        if (g_disableSound == 0) {
            g_gameReg->SetRunState(IsDlgButtonChecked(hDlg, 0x46d));
            i32 mv = GetDialogScrollPosition(hDlg, 0x470);
            if (mv >= 0 && mv <= 100) {
                g_gameReg->SetSoundVolume(mv);
            }
            g_gameReg->m_isVoiceEnabled = IsDlgButtonChecked(hDlg, 0x475);
            i32 sv = GetDialogScrollPosition(hDlg, 0x476);
            if (sv >= 0 && sv <= 100) {
                g_gameReg->SetVoiceVolume(sv);
            }
        }
        if (g_disableAudio == 0 && g_disableMusic == 0 && g_gameReg->m_sound->m_enabled != 0) {
            g_gameReg->SetSoundLevelState(IsDlgButtonChecked(hDlg, 0x471));
            i32 pv = GetDialogScrollPosition(hDlg, 0x472);
            if (pv >= 0 && pv <= 100) {
                g_gameReg->m_sound->SetXMidiVolume(pv);
            }
        }
    }
    i32 qv = GetDialogScrollPosition(hDlg, 0x478);
    if (qv >= 0 && qv <= 100) {
        g_gameReg->m_scrollSpeed = qv;
    }
}

// @early-stop
RVA(0x00036be0, 0xd3)
void ApplyGameOptions() {
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

RVA(0x00036d00, 0x40)
void OnToggleMusicOption(HWND hWnd) {
    if (g_gameReg) {
        i32 state = IsDlgButtonChecked(hWnd, 0x46d);
        g_gameReg->SetRunState(state);
        EnableWindow(GetDlgItem(hWnd, 0x470), state);
    }
}

RVA(0x00036d50, 0x3c)
void OnToggleVoiceOption(HWND hWnd) {
    if (g_gameReg) {
        i32 checked = IsDlgButtonChecked(hWnd, 0x475);
        g_gameReg->m_isVoiceEnabled = checked;
        EnableWindow(GetDlgItem(hWnd, 0x476), checked);
    }
}

RVA(0x00036da0, 0x40)
void OnToggleSpeechOption(HWND hWnd) {
    if (g_gameReg) {
        i32 state = IsDlgButtonChecked(hWnd, 0x471);
        g_gameReg->SetSoundLevelState(state);
        EnableWindow(GetDlgItem(hWnd, 0x472), state);
    }
}

RVA(0x00036df0, 0x1)
void OnToggleCk5Option(HWND__*) {}

RVA(0x00036e10, 0x26)
void OnToggleEasyModeOption(HWND hWnd) {
    if (g_gameReg) {
        g_gameReg->m_isEasyMode = IsDlgButtonChecked(hWnd, 0x455);
    }
}

RVA(0x00036e50, 0x43)
void SetDialogScrollPosition(HWND hDlg, i32 id, i32 pos) {
    HWND h = GetDlgItem(hDlg, id);
    if (h) {
        SCROLLINFO si;
        si.cbSize = 0x1c;
        si.fMask = SIF_POS;
        si.nPos = pos;
        SetScrollInfo(h, SB_CTL, &si, TRUE);
    }
}

RVA(0x00036ec0, 0x41)
i32 GetDialogScrollPosition(HWND hDlg, i32 id) {
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

RVA(0x000370a0, 0xf1)
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo, i32, i32) {
    CWnd* pCtrl = CWnd::FromHandle(static_cast<HWND__*>(hCombo));
    if (!pCtrl) {
        return;
    }

    // The combo-box selection arrives from Windows as a raw LRESULT.
    g_videoResolutionMode = static_cast<Resolution>(SendMessageA(pCtrl->m_hWnd, 0x400, 0, 0));

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

RVA(0x000371e0, 0x5b)
void ConfigureDialogScrollBar(HWND hDlg, i32 id, i32 pos, i32 max) {
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

// @early-stop
RVA(0x00037260, 0x220)
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
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate) {
            return;
        }
        void* cue_ob = 0;
        host->m_cues.Lookup("GAME_VOICE", cue_ob);
        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if (static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
            < static_cast<u32>(cue->m_replayDelay)) {
            return;
        }
        cue->m_lastPlayTime = g_killCueClock;
        cue->m_sound->ConfigureItem(newpos, 0, 0, 0);
        return;
    }
    if (hCtrl == GetDlgItem(hDlg, 0x470)) {
        g_gameReg->SetSoundVolume(newpos);
        if (code == 5) {
            return;
        }
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate) {
            return;
        }
        void* cue_ob = 0;
        host->m_cues.Lookup("GAME_CHIPFALLOUT", cue_ob);
        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
        if (!cue) {
            return;
        }
        if (!g_sndEnabled) {
            return;
        }
        if (static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
            < static_cast<u32>(cue->m_replayDelay)) {
            return;
        }
        cue->m_lastPlayTime = g_killCueClock;
        cue->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
        return;
    }
}

RVA(0x000377e0, 0x6a)
BOOL CALLBACK VideoOptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            DialogInit(hDlg);
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

RVA(0x00037870, 0x3c)
void DialogInit(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    CheckDlgButton(hDlg, 0x46f, g_gameReg->m_isHighDetail);
    CheckDlgButton(hDlg, 0x4d5, g_gameReg->m_isEffectsEnabled);
}

// @early-stop
RVA(0x000378c0, 0x40)
void SaveVideoCheckboxes(HWND hDlg) {
    if (g_gameReg == 0) {
        return;
    }
    g_gameReg->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_gameReg->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
