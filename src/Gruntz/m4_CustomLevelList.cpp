// m4_CustomLevelList.cpp - the "fill the custom-level listbox" dialog helper
// re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// clear listbox 0x3fc, bail if the "Custom" gate is set, then walk the
// custom-level directory glob (_findfirst/_findnext under a shared singleton lock),
// format each entry's display name, ask the settings manager whether to hide it,
// and LB_ADDSTRING it with its 4-char extension stripped. __cdecl(HWND). /GX only
// for the lock's unwind (the per-entry name is a by-value CString the callee
// destroys). Placeholder names; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c) + windows types

#include <Ints.h>
#include <rva.h>
#include <string.h>

namespace m4 {

    // Game Win32 pointer table (reloc-masked indirect calls).
    extern HWND(__stdcall* g_pGetDlgItem)(HWND, int);                       // 0x006c4564
    extern LRESULT(__stdcall* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // CRT-style directory walk (engine copies at these RVAs; name at +0x14).
    struct FindData {
        u32 attrib;      // +0x00
        i32 time_create; // +0x04
        i32 time_access; // +0x08
        i32 time_write;  // +0x0c
        i32 size;        // +0x10
        char name[260];  // +0x14
    };
    extern "C" i32 CrtFindFirst(const char* spec, FindData* fd); // 0x0011f900 (_findfirst)
    extern "C" i32 CrtFindNext(i32 h, FindData* fd);             // 0x0011fa30 (_findnext)
    extern "C" i32 FormatBuf(char* buf, const char* fmt, ...);   // 0x0011f890 (sprintf-ish)
    extern "C" i32 CustomGate(const char* name);                 // 0x0018d290

    // The shared reentrancy lock guarding the directory walk.
    struct WalkLock {
        void Lock();   // 0x001beafb
        void Unlock(); // 0x001beb10
    };
    struct WalkOwner {
        char m_pad0[4];
        WalkLock* m_4; // +0x04
    };
    extern "C" WalkOwner* GetWalkOwner1d3631(); // 0x001d3631

    // The settings-manager query: takes the display name by value (callee destroys).
    struct LevelSettings {
        i32 IsHidden13a2(CString name); // 0x000013a2 (thiscall, by-value CString)
    };
    extern "C" LevelSettings* g_mgrSettings; // 0x0064556c

    // The custom-level glob + display-name format + "already loaded" strings.
    extern char g_customGlob[]; // 0x0060cf94
    extern char g_customDone[]; // 0x0060cf90
    extern char g_nameFmt[];    // 0x0060c5b8

    // @early-stop
    // regalloc + frame-layout wall. Complete correct reconstruction: the listbox
    // reset, the "Custom" gate, the _findfirst glob, the singleton lock (its /GX
    // unwind), the do-while _findnext walk with the per-entry FormatBuf + by-value
    // IsHidden query + extension strip + LB_ADDSTRING, and the unlock all align by
    // shape (llvm-objdump -dr). Residual is MSVC5's stack-buffer placement (the glob
    // vs FindData vs display-name locals land at different [esp+N] than retail's, and
    // the CString-temp arg slot is reused) shifting the operand displacements - not
    // steerable from source.
    RVA(0x0003af90, 0x194)
    i32 FillCustomLevelList(HWND hWnd) {
        HWND lb = g_pGetDlgItem(hWnd, 0x3fc);
        if (!lb) {
            return 0;
        }
        g_pSendMessageA(lb, 0x184, 0, 0); // LB_RESETCONTENT
        if (CustomGate("Custom")) {
            return 0;
        }
        char pattern[260];
        strcpy(pattern, g_customGlob);
        FindData fd;
        i32 h = CrtFindFirst(pattern, &fd);
        i32 found = (h != -1);
        GetWalkOwner1d3631()->m_4->Lock();
        if (found) {
            do {
                char disp[260];
                FormatBuf(disp, g_nameFmt, fd.name);
                if (!g_mgrSettings->IsHidden13a2(CString(disp))) {
                    i32 len = strlen(disp);
                    if (len > 4) {
                        disp[len - 4] = 0;
                    }
                    g_pSendMessageA(lb, 0x180, 0, (LPARAM)disp); // LB_ADDSTRING
                }
            } while (CrtFindNext(h, &fd) != -1);
        }
        CustomGate(g_customDone);
        GetWalkOwner1d3631()->m_4->Unlock();
        return 1;
    }

} // namespace m4
