#ifndef GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H
#define GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H

#include <Mfc.h>

#include <Gruntz/Resolution.h>
#include <Ints.h>

extern i32 g_opt_22bd64;
extern i32 g_opt_22bd68;
extern i32 g_opt_22bd6c;
extern i32 g_opt_22bd70;
extern i32 g_opt_22bd84;
extern i32 g_opt_22bdc4;
extern Resolution g_opt_22bdc8;
extern i32 g_opt_22bdcc;
extern i32 g_opt_22bdd0;
extern i32 g_opt_22bdd4;

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
