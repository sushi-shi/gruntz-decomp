// CustomWorldInfoDlg.cpp - the "CUSTOM_WORLDINFO" custom-world picker dialog helper
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp. Sibling of the CUSTOM_WORLD
// launcher in CustomWorldDialog.cpp (shares the CustomWorldInfoDlgProc + the
// g_pathStr/g_levelStr exchange globals). __cdecl, frameless; helpers are reloc-masked
// externals. Only offsets / code bytes are load-bearing.
#include <Win32.h> // GetDlgItem / SendMessageA / DialogBoxParamA

#include <rva.h>

// The global key-string builders (Set=FUN_001b9e74, Append=FUN_001ba0c8,
// Reset=FUN_001b9c69, the same builder used by BuildPowerupIconKeys).
struct GameKeyStr {
    char* m_str;          // +0x00  the built C-string
    void Set(char* s);    // FUN_001b9e74 __thiscall
    void Append(char* s); // FUN_001ba0c8 __thiscall
    void Reset();         // FUN_001b9c69 __thiscall
};
DATA(0x0022c25c)
extern GameKeyStr g_pathStr; // 0x62c25c  full path builder
DATA(0x0022c260)
extern GameKeyStr g_levelStr; // 0x62c260  level name
DATA(0x0022c26c)
extern HWND g_customWorldParent; // 0x62c26c
DATA(0x0022c270)
extern HINSTANCE g_customWorldInst; // 0x62c270
// FUN_0011fc10 __cdecl: writes the gruntz game dir into buf (ret nonzero on ok).
extern i32 GetGameDir(char* buf, i32 cb);
// FUN_00004282 __cdecl: tests a path exists (OpenFile probe). ret nonzero on ok.
extern i32 PathFileExists(char* path);
// The CUSTOM_WORLDINFO dialog proc (RVA 0x305d), reloc-masked code pointer.
extern "C" INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

// LoadCustomWorldInfo (0x3b7c0) - reads the level name from the dialog's listbox
// (id 0x3fc), builds "<gameDir>\Custom\<level>.WWD" through the global key builders,
// and if the file exists pops the CUSTOM_WORLDINFO dialog.
RVA(0x0003b7c0, 0x12c)
i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = (i32)SendMessageA(hList, 0x188 /*LB_GETCURSEL*/, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if ((i32)SendMessageA(hList, 0x189 /*LB_GETTEXT*/, sel, (LPARAM)szLevel) == -1) {
        return 0;
    }
    g_levelStr.Set(szLevel);
    if (!GetGameDir(szDir, 0xfe)) {
        return 0;
    }
    g_pathStr.Set(szDir);
    g_pathStr.Append("\\Custom\\");
    g_pathStr.Append(szLevel);
    g_pathStr.Append(".WWD");
    if (!PathFileExists(g_pathStr.m_str)) {
        g_pathStr.Reset();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        (DLGPROC)CustomWorldInfoDlgProc,
        0
    );
    return 1;
}
