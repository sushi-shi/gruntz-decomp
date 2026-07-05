// MultiColorDlg.cpp - the multiplayer color-selection dialog item updater
// re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// refresh three dialog items (0x4ff combo, 0x42b, 0x4e9) plus the
// combo's child window. In an active session the items are enabled per an empty-
// slot table probe; out of session the combo's cursel is cleared and the child's
// text is (re)synced to the lobby's current name, then all three are disabled.
// thiscall member, /GX (destructible CString temporaries). Placeholder names;
// only offsets + code bytes are load-bearing. Uses the real MFC CString.
#include <Mfc.h> // real MFC CString (ctor/dtor/strcmp-conversion) + windows types
#ifdef __clang__
// Label-step clang can't parse MFC's afxwin1.inl (implicit-int CMenu::operator==);
// skip the *.inl for clang only - docs/patterns/afxwin-clang-label-step-skip-inl.md.
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h> // the REAL MFC CWnd (dialog items; FromHandle/GetDlgItem/EnableWindow)

#include <Gruntz/Multi.h> // real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Ints.h>
#include <rva.h>
#include <string.h>

namespace m4 {

    // Game Win32 pointer table (0x6c44xx) -> reloc-masked indirect calls.
    extern HWND(WINAPI* g_pGetWindow)(HWND, UINT);                       // 0x006c44d8
    extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // The dialog child items are the real MFC CWnd (<afxwin.h>; the former DlgItem
    // view is folded away): m_hWnd @+0x1c, EnableWindow @0x1be6a7, SetWindowText
    // @0x1be520, GetWindowText(CString&) @0x1bbd01, static FromHandle @0x1bb23a,
    // and the owner's GetItem @0x1be27d == CWnd::GetDlgItem. All out-of-line NAFXCW
    // entrypoints -> reloc-masked.

    // The multiplayer lobby game-state singleton at 0x64bd5c is a CMulti (xref-proven):
    // m_isHost (+0x528) gates the active branch, m_5b0 (+0x5b0) is the current selection,
    // Name42ff/Name31d4 return the current-slot / default name (CString by value).
    extern CMulti* g_64bd5c; // 0x0064bd5c

    // The dialog object itself is a CWnd (GetDlgItem runs on `this`); its two own
    // fields start at +0x5c right past MFC's CDialog span (CWnd is 0x3c) - likely a
    // real CDialog-derived class, based on CWnd here (the evidence-backed minimum).
    struct MultiColorDlg : public CWnd {
        char m_pad3c[0x5c - 0x3c]; // +0x3c..0x5c (the CDialog-member span)
        char* m_5c;                // +0x5c color-slot table base
        char m_pad60[0x6c - 0x60];
        i32 m_6c;            // +0x6c cached selection
        i32 SlotIndex2d4c(); // 0x00002d4c
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
        if (g_64bd5c->m_isHost != 0) {
            CWnd* it4ff = GetDlgItem(0x4ff);
            CWnd* itChild = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
            CWnd* it42b = GetDlgItem(0x42b);
            CWnd* it4e9 = GetDlgItem(0x4e9);
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
        CWnd* it4ff = GetDlgItem(0x4ff);
        CWnd* itChild = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        CWnd* it42b = GetDlgItem(0x42b);
        CWnd* it4e9 = GetDlgItem(0x4e9);
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
        g_pSendMessageA(it4ff->m_hWnd, 0x14e, (WPARAM)-1, 0);
        m_6c = g_64bd5c->m_5b0;
        if (g_64bd5c->m_5b0 != 0) {
            CString name = g_64bd5c->Name42ff();
            itChild->SetWindowText(name);
        } else {
            CString cur;
            itChild->GetWindowText(cur);
            if (strcmp(cur, g_64bd5c->Name31d4()) != 0) {
                itChild->SetWindowText(g_64bd5c->Name31d4());
            }
        }
        it4ff->EnableWindow(0);
        it42b->EnableWindow(0);
        it4e9->EnableWindow(0);
        return 1;
    }

} // namespace m4
