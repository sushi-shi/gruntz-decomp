// VideoConfig.h
#ifndef GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H
#define GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H

#include <Mfc.h> // afx.h FIRST: HWND for the dialog-handler decls below
#include <Ints.h>

extern "C" i32 g_opt_22bd64;
extern "C" i32 g_opt_22bd68;
extern "C" i32 g_opt_22bd6c;
extern "C" i32 g_opt_22bd70;
extern "C" i32 g_opt_22bd84;
extern "C" i32 g_opt_22bdc4;
extern "C" i32 g_opt_22bdc8;
extern "C" i32 g_opt_22bdcc;
extern "C" i32 g_opt_22bdd0;
extern "C" i32 g_opt_22bdd4;

// The dialog handlers. A file-scope prototype has EXTERNAL linkage, so it belongs
// in the owner header rather than being forward-declared in the .cpp; each of
// these carries a real retail RVA, so `static` is not an option.
void LoadGameOptionsToDialog(HWND hDlg);                           // 0x036860
void ReadMenuOptionsDialog(HWND hDlg);                             // 0x036a30
void OnToggleMusicOption(HWND hDlg);                               // 0x036d00
void OnToggleVoiceOption(HWND hDlg);                               // 0x036d50
void OnToggleSpeechOption(HWND hDlg);                              // 0x036da0
void OnToggleEasyModeOption(HWND hDlg);                            // 0x036e10
void OnToggleCk5Option(HWND hDlg);                                 // 0x036df0 (thunk 0x19b5;
void LoadVideoResolutionConfig(HWND hDlg, i32 nIDCombo, i32 nSel); // 0x036f30
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo, i32 code, i32 pos); // 0x0370a0
void ScrollDialog(HWND hDlg, HWND hCtrl, i32 code, i32 pos);               // 0x037260
void DialogInit(HWND hDlg);                                                // 0x037870
void SaveVideoCheckboxes(HWND hDlg);                                       // 0x0378c0
namespace ApiCallerStubs {
    void winapi_0371e0_GetDlgItem_SetScrollInfo(HWND hDlg, i32 id, i32 pos, i32 max); // 0x0371e0
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(HWND hDlg, i32 id);                    // 0x036ec0
} // namespace ApiCallerStubs

#endif // GRUNTZ_GRUNTZ_VIDEOCONFIG_H_H
