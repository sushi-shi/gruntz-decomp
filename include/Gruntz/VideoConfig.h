#ifndef GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H
#define GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H

#include <Mfc.h>

#include <Gruntz/Resolution.h>
#include <Ints.h>

extern b32 g_unusedMusicEnabledSnapshot;
extern i32 g_savedScrollSpeed;
extern i32 g_savedSoundVolume;
extern b32 g_savedEasyMode;
extern b32 g_savedSoundEnabled;
extern i32 g_savedVoiceVolume;
extern Resolution g_savedResolutionMode;
extern i32 g_savedMidiVolume;
extern b32 g_savedMusicEnabled;
extern b32 g_savedVoiceEnabled;

void LoadGameOptionsToDialog(HWND hDlg);
void ReadMenuOptionsDialog(HWND hDlg);
void OnToggleMusicOption(HWND hDlg);
void OnToggleVoiceOption(HWND hDlg);
void OnToggleSpeechOption(HWND hDlg);
void OnToggleEasyModeOption(HWND hDlg);
void OnToggleCk5Option(HWND hDlg);
void LoadVideoResolutionConfig(HWND hDlg, i32 nIDCombo, Resolution nSel);
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo, i32 code, i32 pos);
void ScrollDialog(HWND hDlg, HWND hCtrl, i32 code, i32 pos);

void ApplyGameOptions();
void DialogInit(HWND hDlg);
void SaveVideoCheckboxes(HWND hDlg);

BOOL CALLBACK GameOptionsDlgProc(HWND, UINT, WPARAM, LPARAM);
void ConfigureDialogScrollBar(HWND hDlg, i32 id, i32 pos, i32 max);
i32 GetDialogScrollPosition(HWND hDlg, i32 id);

#endif // GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H
