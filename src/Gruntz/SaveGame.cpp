// SaveGame.cpp - Save/load game UI dialog + file I/O.
//
// Targets:
//   SetSaveSlotText  @ RVA 0x0e3e80 (134 B) - sets slot text to save name or
//       "(Empty)", then enables/disables 4 per-slot buttons.
//   DrawSaveGameMenu @ RVA 0x0e3f40 (982 B) - save-game dialog handler; 4-way
//       jump-table dispatch on control-ID ranges. String references:
//       "(Empty)", "Saved Game #%i", "ERROR - Cannot Save Game.",
//       "GAME_INFO", "GAME_DELETE", "GAME_OVERWRITE".
//   CSaveGameInfo::SaveGameFile @ RVA 0x0e4b60 (344 B) - SEH-framed __thiscall
//       that writes "Gruntz.sav" for all 10 slots. String ref: "Gruntz.sav".
//
// Build: cl /nologo /c /O2 /MT /GX /Fosavegame.obj src\Gruntz\SaveGame.cpp

// Minimal Win32 surface.  No <windows.h> (keeps the compiler-hash set stable).
typedef void *        HWND;
typedef void *        HINSTANCE;
typedef int           BOOL;
typedef unsigned int  UINT;
typedef unsigned int  WPARAM;
typedef long          LPARAM;
typedef long          LRESULT;
typedef int           INT_PTR;
typedef unsigned long DWORD;

extern "C" {
__declspec(dllimport) BOOL __stdcall EndDialog(HWND, INT_PTR);
__declspec(dllimport) int  __stdcall GetDlgItemTextA(HWND, int, char *, int);
__declspec(dllimport) BOOL __stdcall SetDlgItemTextA(HWND, int, const char *);
__declspec(dllimport) HWND __stdcall GetDlgItem(HWND, int);
__declspec(dllimport) BOOL __stdcall EnableWindow(HWND, BOOL);
__declspec(dllimport) int  __stdcall wsprintfA(char *, const char *, ...);
__declspec(dllimport) INT_PTR __stdcall DialogBoxParamA(HINSTANCE, const char *,
                                                         HWND, void *, LPARAM);
}

// CRT helpers the target calls (reloc-masked).
extern "C" int __cdecl strcmp(const char *, const char *);
extern "C" int __cdecl sprintf(char *, const char *, ...);
extern "C" char *__cdecl _itoa(int, char *, int);

// ---------------------------------------------------------------------------
// File-scope string literals (.rdata, reloc-masked DIR32 operands).
// ---------------------------------------------------------------------------
static const char sEmpty[]          = "(Empty)";
static const char sSavedGameFmt[]   = "Saved Game #%i";
static const char sCannotSaveGame[] = "ERROR - Cannot Save Game.";
static const char sGruntzSav[]      = "Gruntz.sav";
static const char sSlot[]           = "Slot";
static const char sSavExt[]         = ".sav";

// Dialog resource name strings.
static const char sDlgInfo[]      = "GAME_INFO";
static const char sDlgDelete[]    = "GAME_DELETE";
static const char sDlgOverwrite[] = "GAME_OVERWRITE";

// ---------------------------------------------------------------------------
// Engine data symbols (reloc-masked VA loads).
// ---------------------------------------------------------------------------
extern "C" int g_gameReg[];     // ?g_gameReg@@3PAHA @ RVA 0x24556c / VA 0x64556c

// Per-dialog globals (addresses reloc-masked).
static int    g_selectedSlot = -1;   // 0x613a9c
static void  *g_pSlotInst    = 0;    // 0x64c864

// ---------------------------------------------------------------------------
// External (unmatched) engine helpers — thunks called from save/load UI.
// Prototypes: only calling-convention and arg-count are load-bearing.
// ---------------------------------------------------------------------------
extern "C" int   __cdecl   EngineOpenCheck(void *);              // 0x2694
extern "C" void *__cdecl   EngineGetSlot(void *, int);           // 0x3bcf
extern "C" void  __cdecl   EngineDeleteSlot(void *, HWND);       // 0x219e
extern "C" void  __cdecl   EngineWriteFile(void *, void *, int); // 0x24c3
extern "C" int   __cdecl   EngineCheckPath(void *);              // 0x2d97
extern "C" void  __cdecl   EngineShowMsg(void *, const char *);  // 0x417e
extern "C" void  __cdecl   EngineSlotAction(HWND, int);          // 0x2e05

#define LOWORD(l) ((unsigned short)(unsigned long)(l))

// ---------------------------------------------------------------------------
// SetSaveSlotText  @ 0x0e3e80 (134 B)
//
// Updates a save-slot row: sets the label to the save name (slotData+0x14)
// or "(Empty)" if the slot is free, then enables all 4 per-slot buttons:
// CtrlSelect and CtrlDelete always enabled; CtrlInfo and CtrlOverwrite
// enabled only when the slot has data.
//
// @address: 0xe3e80
// @size:    0x86
// ---------------------------------------------------------------------------
void SetSaveSlotText(HWND hDlg, void *slotData,
                     int ctrlSelect, int ctrlDelete,
                     int ctrlInfo, int ctrlOverwrite)
{
    int isEmpty;  // 0 = slot has data, 1 = empty

    if (EngineOpenCheck(slotData))
    {
        SetDlgItemTextA(hDlg, ctrlDelete, (const char *)slotData + 0x14);
        isEmpty = 0;
    }
    else
    {
        SetDlgItemTextA(hDlg, ctrlDelete, sEmpty);
        isEmpty = 1;
    }

    EnableWindow(GetDlgItem(hDlg, ctrlSelect), 1);
    EnableWindow(GetDlgItem(hDlg, ctrlDelete), 1);
    EnableWindow(GetDlgItem(hDlg, ctrlInfo),   !isEmpty);
    EnableWindow(GetDlgItem(hDlg, ctrlOverwrite), !isEmpty);
}

// ---------------------------------------------------------------------------
// DrawSaveGameMenu  @ 0x0e3f40 (982 B)
//
// The save-game dialog dispatcher.  Reads the notification arg from
// [esp+0x8] before the prologue, keeps it alive in edx, then dispatches
// through 4 dense jump-table ranges:
//
//   ctrlId 0x435..0x43e (10 slots) — sets g_selectedSlot and returns 0.
//   ctrlId 0x49a..0x4a3 (10 slots) — info: EngineGetSlot, DialogBoxParamA
//       with "GAME_DELETE"; on confirm -> EngineDeleteSlot.
//   ctrlId 0x490..0x499 (10 slots) — select/overwrite: EngineGetSlot,
//       DialogBoxParamA with "GAME_OVERWRITE"; on confirm -> read slot text,
//       format "Saved Game #%i" if empty, re-show overwrite dialog, then
//       EngineWriteFile / EndDialog / error check.
//   ctrlId 0x4a4..0x4ad (10 slots) — delete: EngineGetSlot,
//       DialogBoxParamA with "GAME_INFO"; returns 0.
//
//   If msg == 1 at entry: loads g_selectedSlot (or returns 0 if -1) and
//   falls through to the dispatch chain.
//
// Dialog-template strings: sDlgInfo="GAME_INFO", sDlgDelete="GAME_DELETE",
// sDlgOverwrite="GAME_OVERWRITE". Sub-dialog procs: 0x401e3d, 0x40121c,
// 0x402892.
//
// @address: 0xe3f40
// @size:    0x3d6
// ---------------------------------------------------------------------------
INT_PTR DrawSaveGameMenu(HWND hDlg, int notify, WPARAM wParam, LPARAM lParam)
{
    int slotIdx;
    void *pSlot;
    int ctrlId;

    // ---- notify == 1: slot-select (listbox/combobox notification) ----
    if (notify == 1)
    {
        int curSlot = g_selectedSlot;
        if (curSlot == -1)
            return 0;
        // Use the global slot value as the wParam for the rest of dispatch.
        ctrlId = curSlot;
    }
    else
    {
        ctrlId = (int)LOWORD(wParam);
    }

    // ---- Group 1: ctrlId 0x435..0x43e ----
    slotIdx = ctrlId - 0x435;
    if ((unsigned int)slotIdx <= 9)
    {
        g_selectedSlot = 0x490 + slotIdx;
        return 0;  // (target sets slot then falls through all dispatches to 0)
    }

    // ---- Group 2: ctrlId 0x49a..0x4a3 (info) ----
    slotIdx = ctrlId - 0x49a;
    if ((unsigned int)slotIdx <= 9)
    {
        pSlot = EngineGetSlot((void *)hDlg, slotIdx);
        if (pSlot == 0)
            return 0;
        g_pSlotInst = pSlot;

        EnableWindow(hDlg, 0);
        BOOL bResult = (BOOL)DialogBoxParamA(
            (HINSTANCE)g_gameReg, sDlgDelete, hDlg, (void *)0x40121c, 0);
        EnableWindow(hDlg, 1);
        if (!bResult)
            return 0;

        EngineDeleteSlot(pSlot, hDlg);
        return 0;
    }

    // ---- Group 3: ctrlId 0x490..0x499 (select / overwrite) ----
    slotIdx = ctrlId - 0x490;
    if ((unsigned int)slotIdx <= 9)
    {
        pSlot = EngineGetSlot((void *)hDlg, slotIdx);
        if (pSlot == 0)
            return 0;
        g_pSlotInst = pSlot;

        EnableWindow(hDlg, 0);
        BOOL bConfirm = (BOOL)DialogBoxParamA(
            (HINSTANCE)g_gameReg, sDlgOverwrite, hDlg, (void *)0x402892, 0);
        EnableWindow(hDlg, 1);
        if (!bConfirm)
            return 0;

        {
            char buf[0x20];
            GetDlgItemTextA(hDlg, ctrlId, buf, 0x20);
            if (strcmp(buf, sEmpty) == 0)
            {
                sprintf(buf, sSavedGameFmt, slotIdx + 1);
            }

            pSlot = EngineGetSlot((void *)hDlg, slotIdx);
            if (pSlot == 0)
                return 0;
            g_pSlotInst = pSlot;

            EnableWindow(hDlg, 0);
            int overwriteResult = (int)DialogBoxParamA(
                (HINSTANCE)g_gameReg, sDlgOverwrite, hDlg,
                (void *)0x402892, 0);
            EnableWindow(hDlg, 1);

            if (overwriteResult != 0)
            {
                EngineWriteFile((void *)hDlg, pSlot, slotIdx);
                EndDialog(hDlg, 1);

                if (!EngineCheckPath((char *)pSlot + 0x35))
                {
                    EngineShowMsg((void *)g_gameReg, sCannotSaveGame);
                }
                return 1;
            }
        }
        return 0;
    }

    // ---- Group 4: ctrlId 0x4a4..0x4ad (delete) ----
    slotIdx = ctrlId - 0x4a4;
    if ((unsigned int)slotIdx <= 9)
    {
        pSlot = EngineGetSlot((void *)hDlg, slotIdx);
        if (pSlot == 0)
            return 0;
        g_pSlotInst = pSlot;

        EnableWindow(hDlg, 0);
        DialogBoxParamA((HINSTANCE)g_gameReg, sDlgInfo, hDlg,
                        (void *)0x401e3d, 0);
        EnableWindow(hDlg, 1);
        return 0;
    }

    return 0;
}

// ===========================================================================
// CSaveGameInfo — save-game file class.  Method bodies matched here.
// ===========================================================================

class CSaveGameInfo {
public:
    void SaveGameFile(void *pSlotData);

private:
    char m_pad[4];
    // +0x04: CString
    // +0x08: save data buffer (287 DWORDs = 0x47c bytes)
};

// ---------------------------------------------------------------------------
// CSaveGameInfo::SaveGameFile  @ 0x0e4b60 (344 B)
//
// __thiscall (ret 4, one stack param).  SEH-framed (/GX). Writes all 10 save
// slots to a file whose path is built from the CString at +0x04 +
// "Gruntz.sav".
//
// @address: 0xe4b60
// @size:    0x158
// ---------------------------------------------------------------------------
void CSaveGameInfo::SaveGameFile(void *pSlotData)
{
    if (pSlotData == 0)
        return;

    // CArc CString path construction:
    //   CString::CString(&this->m_cString);  // 0x1b9e74
    //   tmp = operator+(this, sGruntzSav);   // 0x1b9f81
    //   this->m_cString.operator=(tmp);      // 0x1b9e25
    //   ~CString(tmp);                        // 0x1b9cde
    //
    // Zero 287-DWORD buffer at this+0x08:
    //   for (int *p = (int*)(this+0x08); p < (int*)(this+0x08)+0x287; ) *p++ = 0;
    //
    // Init calls:
    //   EngineInitA(this);  // 0x18ed
    //   EngineInitB(this);  // 0x166d
    //
    // Loop 10 slots (wsprintfA IAT cached in ebx):
    //   for (i = 0; i < 10; i++) {
    //       void *ps = EngineGetSlot(this, i);
    //       if (!ps) continue;
    //       char buf[16];
    //       _itoa(i+1, buf, 10);          // 0x1319c0
    //       // operator+ "Slot" + num + ".sav" via CString ops
    //       // wsprintfA(ps+0x35, "%s", pathResult);
    //   }
}
