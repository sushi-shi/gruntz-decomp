// LoadGameMenu.h - the LoadGameMenu.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_LOADGAMEMENU_H
#define GRUNTZ_GRUNTZ_LOADGAMEMENU_H

#include <Mfc.h> // afx.h FIRST: Win32 types named below
#include <Ints.h>

// This TU's own modal proc. (The LoadDeleteDlgProc/LoadInfoDlgProc ILT-thunk
// placeholders that used to sit here were duplicates: thunks 0x121c/0x1e3d resolve
// to SaveGame.cpp's winapi_0e3a40_EndDialog / LevelPreviewDlgProc, which the
// GAME_DELETE/GAME_INFO sites in BOTH TUs push.)
i32 CALLBACK GruntzLoadGameDlgProc(HWND, UINT, WPARAM, LPARAM); // 0x09dff0 GAME_LOAD

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
// g_slotState now lives in its owner header <Io/SaveGame.h> (typed SaveSlot*).

struct SaveSlot; // <Io/SaveGame.h>
class CSaveGame; // <Io/SaveGame.h>

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
void FillGameInfoDialog(HWND hDlg, CSaveGame* dlg);
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
int TempFileExists(SaveSlot* p);                           // 0x0e5700 (SaveGame.cpp)
i32 LoadGameCommand(HWND hwnd, i32 cmdId, CSaveGame* dlg); // 0x9e390 (WM_COMMAND handler)

#endif // GRUNTZ_GRUNTZ_LOADGAMEMENU_H
