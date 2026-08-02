#ifndef GRUNTZ_GRUNTZ_LOADGAMEMENU_H
#define GRUNTZ_GRUNTZ_LOADGAMEMENU_H

#include <Mfc.h>

#include <Ints.h>

i32 CALLBACK GruntzLoadGameDlgProc(HWND, UINT, WPARAM, LPARAM);

struct SaveSlot;
class CSaveGame;

void FillGameInfoDialog(HWND hDlg, CSaveGame* dlg);
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);

int TempFileExists(SaveSlot* p);
i32 LoadGameCommand(HWND hwnd, i32 cmdId, CSaveGame* dlg);

#endif // GRUNTZ_GRUNTZ_LOADGAMEMENU_H
