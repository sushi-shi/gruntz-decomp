// m4_MultiColorDlg.cpp - the multiplayer color-selection dialog item updater
// re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// 0x000c1aa0: refresh three dialog items (0x4ff combo, 0x42b, 0x4e9) plus the
// combo's child window. In an active session the items are enabled per an empty-
// slot table probe; out of session the combo's cursel is cleared and the child's
// text is (re)synced to the lobby's current name, then all three are disabled.
// thiscall member, /GX (destructible CString temporaries). Placeholder names;
// only offsets + code bytes are load-bearing. Uses the real MFC CString.
#include <Mfc.h> // real MFC CString (ctor/dtor/strcmp-conversion) + windows types

#include <Ints.h>
#include <rva.h>
#include <string.h>

namespace m4 {

    // Game Win32 pointer table (0x6c44xx) -> reloc-masked indirect calls.
    extern HWND(__stdcall* g_pGetWindow)(HWND, UINT);                       // 0x006c44d8
    extern LRESULT(__stdcall* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // A dialog child item (CWnd-ish); its HWND lives at +0x1c. All methods are
    // out-of-line (MFC / other TUs) -> reloc-masked.
    struct DlgItem {
        char m_pad00[0x1c];
        HWND m_1c;                       // +0x1c
        void EnableWindow(i32 en);       // 0x001be6a7
        void SetText(const char* s);     // 0x001be520 (SetWindowText)
        void GetText(CString& out);      // 0x001bbd01 (GetWindowText)
    };
    DlgItem* __stdcall FromHandle1bb23a(HWND h); // 0x001bb23a (CWnd::FromHandle)

    // The multiplayer lobby manager singleton (g_64bd5c). m_5c-relative color-slot
    // table is probed per active player; Name42ff/Name31d4 return CString by value.
    struct LobbyMgr {
        char m_pad00[0x528];
        i32 m_528; // +0x528 session-active flag
        char m_pad52c[0x5b0 - 0x52c];
        i32 m_5b0; // +0x5b0 current selection
        CString Name42ff(); // 0x000042ff (thiscall, returns CString by value)
        CString Name31d4(); // 0x000031d4
    };
    extern LobbyMgr* g_64bd5c; // 0x0064bd5c

    struct MultiColorDlg {
        char m_pad00[0x5c];
        char* m_5c; // +0x5c color-slot table base
        char m_pad60[0x6c - 0x60];
        i32 m_6c; // +0x6c cached selection
        DlgItem* GetItem(i32 id); // 0x001be27d
        i32 SlotIndex2d4c();      // 0x00002d4c
        i32 UpdateColorItems();
    };

    // @early-stop
    // regalloc + EH-state wall. Complete correct reconstruction: the session-active
    // branch (four GetItem/child fetches, the null-guard chain, the empty-slot table
    // probe SlotIndex*71*8+0x16c and the enable/disable) and the out-of-session
    // branch (CB_SETCURSEL -1, the m_5b0 gate, the two by-value CString name fetches,
    // the inline strcmp resync, and the disable trio) all align by shape (llvm-objdump
    // -dr). Residual is MSVC5 permuting the four item pointers across edi/ebp/ebx and
    // the dead arg/temp stack slots between the two branches, shifting [esp+N] operands
    // - plus the demangled-vs-mangled MFC/CString reloc names - not steerable.
    RVA(0x000c1aa0, 0x2f8)
    i32 MultiColorDlg::UpdateColorItems() {
        if (g_64bd5c->m_528 != 0) {
            DlgItem* it4ff = GetItem(0x4ff);
            DlgItem* itChild = FromHandle1bb23a(g_pGetWindow(GetItem(0x4ff)->m_1c, 5));
            DlgItem* it42b = GetItem(0x42b);
            DlgItem* it4e9 = GetItem(0x4e9);
            if (!itChild) {
                return 0;
            }
            if (!it4ff) {
                return 0;
            }
            if (!it42b) {
                return 0;
            }
            if (!it4e9) {
                return 0;
            }
            i32 idx = SlotIndex2d4c();
            i32 en = (*(i32*)(m_5c + idx * 568 + 0x16c) == 0);
            it4ff->EnableWindow(en);
            it42b->EnableWindow(en);
            it4e9->EnableWindow(0);
            return 1;
        }
        DlgItem* it4ff = GetItem(0x4ff);
        DlgItem* itChild = FromHandle1bb23a(g_pGetWindow(GetItem(0x4ff)->m_1c, 5));
        DlgItem* it42b = GetItem(0x42b);
        DlgItem* it4e9 = GetItem(0x4e9);
        if (!itChild) {
            return 0;
        }
        if (!it4ff) {
            return 0;
        }
        if (!it42b) {
            return 0;
        }
        if (!it4e9) {
            return 0;
        }
        g_pSendMessageA(it4ff->m_1c, 0x14e, (WPARAM)-1, 0);
        m_6c = g_64bd5c->m_5b0;
        if (g_64bd5c->m_5b0 != 0) {
            CString name = g_64bd5c->Name42ff();
            itChild->SetText(name);
        } else {
            CString cur;
            itChild->GetText(cur);
            if (strcmp(cur, g_64bd5c->Name31d4()) != 0) {
                itChild->SetText(g_64bd5c->Name31d4());
            }
        }
        it4ff->EnableWindow(0);
        it42b->EnableWindow(0);
        it4e9->EnableWindow(0);
        return 1;
    }

} // namespace m4
