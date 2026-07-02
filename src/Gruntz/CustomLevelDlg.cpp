// CustomLevelDlg.cpp - the Battlez/custom-level dialog helper re-homed out of
// src/Stub/ApiCallers.cpp (0x000180e0). __thiscall on the dialog host: when the
// incoming flag is 0 it (re)fills the custom-level listbox (item 0x516) by walking
// "<gamedir>\custom\*.wwd" under the shared reentrancy lock, prefixing each match
// with a function-static CString("custom\\"), asking the settings manager whether
// to show it, and LB_ADDSTRING-ing it; when the flag is non-zero it reads the
// current listbox selection's text into the host's +0x5c CString and post-processes
// it. /GX for the by-value CString arg + the local CString glob unwind. Placeholder
// names; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (ctor 0x1b9d4c, operator+= 0x1ba0c8, operator+ 0x1b9f81)

#include <Ints.h>
#include <rva.h>

namespace m4dlg {

    // The listbox / dialog item the host resolves (its HWND lives at +0x1c).
    struct WndItem {
        char m_pad0[0x1c];
        HWND m_hwnd; // +0x1c
        // copy the item text at index `sel` into the host's CString.
        void GetText1ce692(i32 sel, void* out);
    };

    // The post-select processor run on the host's +0x5c CString.
    struct SelName {
        char m_pad0[4];
        void Proc1ba24c(); // 0x001ba24c (thiscall, no args)
    };

    // The dialog host (this).
    struct CustomLevelDlg {
        i32 Populate180e0(i32* pFlag);  // 0x000180e0
        WndItem* GetItem1be27d(i32 id); // 0x001be27d (thiscall)
        char m_pad0[0x5c];
        SelName m_5c; // +0x5c (selected-map name)
    };

    // Game Win32 pointer table (reloc-masked indirect call).
    extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // CRT-style directory walk (engine copies at these RVAs; name at +0x14).
    struct FindData {
        u32 attrib;      // +0x00
        i32 time_create; // +0x04
        i32 time_access; // +0x08
        i32 time_write;  // +0x0c
        i32 size;        // +0x10
        char name[260];  // +0x14
    };
    extern "C" i32 CrtFindFirst(const char* spec, FindData* fd); // 0x0011f900
    extern "C" i32 CrtFindNext(i32 h, FindData* fd);             // 0x0011fa30
    extern "C" void GetGameDir(char* buf, i32 size);             // 0x0011fc10

    // The shared reentrancy lock guarding the directory walk.
    struct WalkLock {
        i32 Lock();   // 0x001beafb
        i32 Unlock(); // 0x001beb10
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

    // @early-stop
    // stack-buffer-placement wall (same as sibling m4::FillCustomLevelList @0x3af90):
    // complete correct reconstruction - the GetItem gate, the shared lock's /GX
    // unwind, the gamedir "\custom\*.wwd" glob, the function-static CString("custom\\")
    // magic-static guard + atexit, the loop-rotated _findfirst/_findnext walk with the
    // per-entry operator+ + by-value IsHidden query + LB_ADDSTRING, the LB finalize
    // and unlock, and the flag!=0 select branch all align by shape (llvm-objdump -dr).
    // Residual is MSVC5's [esp+N] local placement (glob / FindData / CString-temp arg
    // slots land differently than retail) + the by-value CString-temp lifetime - not
    // steerable from source.
    RVA(0x000180e0, 0x23f)
    i32 CustomLevelDlg::Populate180e0(i32* pFlag) {
        WndItem* item = GetItem1be27d(0x516);
        if (*pFlag == 0) {
            GetWalkOwner1d3631()->m_4->Lock();
            {
                char buf[0x400];
                GetGameDir(buf, 0x400);
                CString glob(buf);
                glob += "\\custom\\*.wwd";
                FindData fd;
                i32 h = CrtFindFirst(glob, &fd);
                static CString s_custom("custom\\");
                if (h != -1) {
                    do {
                        if (g_mgrSettings->IsHidden13a2(s_custom + fd.name)) {
                            g_pSendMessageA(
                                item->m_hwnd,
                                0x180,
                                0,
                                (LPARAM)(const char*)(s_custom + fd.name)
                            );
                        }
                    } while (CrtFindNext(h, &fd) != -1);
                }
                g_pSendMessageA(item->m_hwnd, 0x186, 0, 0);
            }
            return GetWalkOwner1d3631()->m_4->Unlock();
        }
        i32 sel = (i32)g_pSendMessageA(item->m_hwnd, 0x188, 0, 0);
        if (sel == -1) {
            return sel;
        }
        item->GetText1ce692(sel, &m_5c);
        m_5c.Proc1ba24c();
        return 0;
    }

} // namespace m4dlg
