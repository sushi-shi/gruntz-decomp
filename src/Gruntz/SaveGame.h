// SaveGame.h - Save/load game UI + file I/O declarations.

#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
// CSaveGameInfo — the save-game file class.
//
// Layout from the matched functions:
//   +0x00: (base / vftable)
//   +0x04: CString path builder
//   +0x08: save-data buffer (287 DWORDs = 0x47c bytes)
// ---------------------------------------------------------------------------
class CSaveGameInfo {
public:
    // Writes all 10 save slots to "Gruntz.sav".  SEH-framed (/GX).
    // @address: 0xe4b60
    // @size:    0x158
    void SaveGameFile(void *pSlotData);

private:
    char m_pad[0x04];      // +0x00 base/vftable placeholder
    // +0x04: CString (opaque, 0x10 bytes typically)
    // +0x08: data[0x47c] (287 DWORDs)
};

// ---------------------------------------------------------------------------
// Free functions.
// ---------------------------------------------------------------------------

// Updates a save-slot row in the save-game dialog.
// @address: 0xe3e80
// @size:    0x86
void SetSaveSlotText(HWND hDlg, void *slotData,
                     int ctrlSelect, int ctrlDelete,
                     int ctrlInfo, int ctrlOverwrite);

// The save-game dialog window procedure.
// @address: 0xe3f40
// @size:    0x3d6
INT_PTR DrawSaveGameMenu(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
