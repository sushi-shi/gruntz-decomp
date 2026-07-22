#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <EmptyString.h> // g_emptyString
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Random.h> // g_randSeed/g_randSeeded (FlashCtrlD's swatch colour)
#include <rva.h>
#include <string.h> // inline strcmp (the empty-text WM_SETTEXT gate in the edit subclass)

DATA(0x001e8d10)
const i32 g_msgmap_CBattlezDlgColors = 6205544;

RVA(0x00014b10, 0x5)
long CBattlezDlg::DoDefault() {
    return Default();
}

RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(CGruntzMgr* a0, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_slots = a0;
    m_customNameFlag = 0;
}

// ~CBattlezDlg @0x14c90 - destroy the CString member m_6c, then chain the NAFXCW ~CDialog
// base dtor (both reloc-masked), under a /GX EH frame that unwinds the half-torn object
// across the member dtor.
//
// IT IS COMPILER-GENERATED, so there is no definition here to hang an RVA() on - the class
// declares no dtor (see <Gruntz/Dialogs.h>) and cl emits the COMDAT because the vtable slot
// needs its address. That is not a workaround, it is what the bytes say: retail's 71-byte
// body has NO `mov [esi],??_7CBattlezDlg` re-stamp at entry, and cl emits that stamp for
// every user-written dtor body. Declaring the dtor cost exactly those 6 bytes (the old
// "vptr-restamp-presence wall", ~94.4%) and forced GruntzMgr.cpp to carry a second
// definition of the class under the same name. Both are gone.
RVA_COMPGEN(0x00014c90, 0x47, ??1CBattlezDlg@@UAE@XZ)

// @confidence: low
// @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
// @early-stop
// CBattlezDlg::DoDataExchange @0x14d00 - ??_7CBattlezDlg (0x1e8bac) slot 35, +0x8c:
// the MFC DDX of the Battlez multiplayer-setup dialog (GAME code, 2664 B). Reads config
// via g_buteMgr ("Battlez_Setup" section: LastMaxGruntz%d / LastDiff%d / LastColour%d,
// DefaultMaxGruntz) + g_gameReg, populates the dialog controls (the "Computer
// (easy/normal/difficult)", "Human", "Player", "Serra", "Jebediah" combo/list strings)
// and drives them via the ::SendMessageA / PTR_GetWindow / PTR_GetWindowLongA /
// PTR_SetWindowLongA function-pointer trampolines - which is exactly what a DDX does.
// WIRED (VT1): was the free fn `BattlezSetupDlgInit` (a Ghidra name guess, RVA-homed
// from src/Stub/ApiCallers.cpp) while THIS class's own `virtual void DoDataExchange
// (CDataExchange*) OVERRIDE // slot 35` had no definition - the two sat unjoined in the
// same file. Slot identity is unambiguous: both sibling dialogs put their real
// DoDataExchange at slot 35 (CBattlezDlgColors 0x179b0, CBattlezDlgCustom 0x180e0), and
// retail 0x14d00 ends `ret 0x4` == a __thiscall with one pointer arg, i.e. CDataExchange*.
// Body still parked (>512B leaf-first: ~20 CButeMgr/CString/CGameReg callees + a
// subclass window trampoline must be modeled first) - the BINDING is fixed, not the
// byte-match.
RVA(0x00014d00, 0xa68)
void CBattlezDlg::DoDataExchange(CDataExchange* pDX) {}

// ---------------------------------------------------------------------------
// The MFC GDI COMDAT pool this TU emits (its code lands between the CBattlezDlg
// ctor and CBattlezDlgCustom). OnDrawItem's stack CDC/CBrush make cl instantiate
// the inline MFC dtors out-of-line for the vtables + /GX unwind funclets, and the
// linker kept THIS TU's copies (first CBrush user in link order; MSVC5 keeps one
// COMDAT per mangled name). RTTI ground truth for the vtables they stamp:
//   0x1e8cb4 = ??_7CObject@@6B@      0x1e8cd4 = ??_7CGdiObject@@6B@
//   0x1e8cf4 = ??_7CBrush@@6B@       (auto-named via config/vtable_names.csv when
// this obj emits the ??_7 COMDATs). Byte-proof: a scratch /GX+/O2 TU with a stack
// CBrush reproduces every body below exactly (mod relocs). The former fake
// CImgHolderBase/CImgHolder2/CImgHolder hierarchy (+ the "CImageList holder"
// story) is RTTI-refuted (see the header note above).
RVA_COMPGEN(0x000163e0, 0x1e, ??_GCObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016410, 0x7, ??1CObject@@UAE@XZ)
RVA_COMPGEN(0x00016430, 0x1e, ??_GCGdiObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016460, 0x46, ??1CGdiObject@@UAE@XZ)
// ??_GCBrush @0x164d0 + ??_7CBrush @0x1e8cf4 are forced by the INLINE CBrush
// default ctor in FlashCtrlD's scratch brush (0x160f0, below) - which is part of
// THIS retail .obj. (It used to be split out into a FlashRect.cpp; that split was
// artificial - 0x160f0 sits inside this TU's own 0x14b30..0x18086 block - and has
// been reunited here, so this obj emits the ??_G/??_7 pair as retail's did.)
RVA_COMPGEN(0x000164d0, 0x1e, ??_GCBrush@@UAEPAXI@Z)
RVA_COMPGEN(0x00016500, 0x46, ??1CBrush@@UAE@XZ)

// ~CBattlezDlgCustom @0x17140 - destroy the CString member m_customName, then chain the
// NAFXCW ~CDialog base dtor, under a /GX EH frame for the member unwind. COMPILER-GENERATED
// (see <Gruntz/Dialogs.h>), so there is no definition to hang an RVA() on: retail's body
// carries no `mov [esi],??_7CBattlezDlgCustom` re-stamp, and cl only emits that for a
// user-written dtor body. Declaring it (even `inline ... {}`) added exactly that stamp -
// a "vptr-restamp-presence wall" that capped both this body (~94.4%) and the copy
// ShowCustomDlg inlines (~92.9%).
RVA_COMPGEN(0x00017140, 0x47, ??1CBattlezDlgCustom@@UAE@XZ)

RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}

VTBL(CBattlezDlg, 0x001e8bac); // vtable_names -> code (RTTI game class)
VTBL(CBattlezDlgColors, 0x001e8d94); // vtable_names -> code (RTTI game class)
VTBL(CBattlezDlgCustom, 0x001e8ee4); // vtable_names -> code (RTTI game class)
DATA(0x00229d10)
WNDPROC g_savedDlgWndProc; // the saved original proc (was i32; no writer in src - DATA-only)

RVA(0x00015a10, 0x70)
i32 CALLBACK WndProc_15a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {
        if (strcmp(g_emptyString, reinterpret_cast<const char*>(lParam)) == 0) {
            return 0;
        }
    }
    return CallWindowProcA(g_savedDlgWndProc, hWnd, msg, wParam, lParam);
}

// ShowCustomDlg (0x17030) - stack-construct a CBattlezDlgCustom and DoModal it;
// on IDOK, if its custom-name CString m_customName is non-empty, uppercase it and shove it
// into the child window of the 0x4ff combo (GetWindow(GW_CHILD) -> FromHandle ->
// SetWindowText), then latch m_68. The /GX EH frame + inlined ~CBattlezDlgCustom
// (member ~CString m_customName + base ~CDialog) unwind the local dialog. The dialog ctor
// (0x1ccb thunk), DoModal, MakeUpper, GetDlgItem, FromHandle, SetWindowText, and
// the CString ctor/dtor are all NAFXCW/thunk calls that reloc-mask. Marking
// ~CBattlezDlgCustom `inline` is what makes /Ob1 inline the teardown here (retail
// inlined it too - separate ~CString + ~CDialog calls, not one ??1 call).
// @early-stop
// EH-TRYLEVEL wall (~90.7%). The vptr-restamp half of this wall is FIXED: making
// ~CBattlezDlgCustom compiler-generated (the binary's own evidence - retail's dtor carries
// no re-stamp, and the out-of-line COMDAT is now byte-EXACT) removed the spurious
// `mov [esp+4],&??_7CBattlezDlgCustom`. What remains is the /GX bookkeeping around the
// inlined teardown: retail numbers its trylevels 0/1/2/-1 where cl emits 0/1/-1, and the
// `child != 0` branch polarity flips with it. The ctor/DoModal/GetLength/MakeUpper/
// GetDlgItem/GetWindow/FromHandle/SetWindowText chain is byte-exact. (Score moved 92.9 ->
// 90.7 when the stamp went: the extra instruction had been padding the alignment of the
// tail it now no longer shifts - a scoring artifact, not a regression; the dtor it shares
// with 0x17140 went 94.4 -> 100.) Not source-steerable; final sweep.
RVA(0x00017030, 0xc1)
void CBattlezDlg::ShowCustomDlg() {
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1) {
        if (dlg.m_customName.GetLength() != 0) {
            dlg.m_customName.MakeUpper();
            CWnd* item = GetDlgItem(0x4ff);
            CWnd* child = CWnd::FromHandle(::GetWindow(item->m_hWnd, GW_CHILD));
            if (child != 0) {
                child->SetWindowTextA(dlg.m_customName);
                m_customNameFlag = 1;
            }
        }
    }
}

RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(CGruntzMgr* a0, i32 a1, i32 a2, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_slots = a0;
    m_slotIndex = a1;
    m_pickedColor = 0;
    m_68 = a2;
}

// CBattlezDlgColors::DoDataExchange (0x179b0): the MFC DDX for the colour-picker
// listbox (control 0x515). SAVE (m_bSaveAndValidate): read the selected item's
// data (the colour index) into m_pickedColor, clamped to 0x10. LOAD: for each of
// the 17 colours, mark it taken if any of the 4 occupied player slots (m_slots,
// stride 0x238) already uses it; add the free ones as "Color" items (item-data =
// colour index), then select the first. GetDlgItem(0x515) runs in both branches
// (the compiler hoists the shared push of the control id).
// @early-stop
// 99.5%: logic byte-exact. Residual is a 2-register coin-flip in the LOAD branch -
// retail assigns pSend->edi / lb->ebx, cl assigns pSend->ebx / lb->edi (esi is the
// loop counter in both). A pure callee-saved allocation choice; permuter + source
// reorder tried, not steerable.
RVA(0x000179b0, 0xcb)
void CBattlezDlgColors::DoDataExchange(CDataExchange* pDX) {
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM);
    if (pDX->m_bSaveAndValidate) {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        long sel = pSend(lb->m_hWnd, 0x188, 0, 0);    // LB_GETCURSEL
        long data = pSend(lb->m_hWnd, 0x199, sel, 0); // LB_GETITEMDATA
        m_pickedColor = data;
        if (data >= 0x11) {
            m_pickedColor = 0x10;
        }
    } else {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        for (i32 i = 0; i < 0x11; i++) {
            i32 avail = 1;
            GruntzPlayer* rec = m_slots->m_options; // the per-player slots (color=m_008, occupancy=m_liveGate)
            for (i32 j = 0; j < 4; j++) {
                if (rec->m_liveGate != 0 && rec->m_008 == i) { // occupied slot already using color i
                    avail = 0;
                }
                rec++;
            }
            if (avail) {
                long idx = pSend(lb->m_hWnd, 0x180, 0, reinterpret_cast<long>("Color")); // LB_ADDSTRING
                pSend(lb->m_hWnd, 0x19a, idx, i);                      // LB_SETITEMDATA
            }
        }
        pSend(lb->m_hWnd, 0x186, 0, 0); // LB_SETCURSEL
    }
}

RVA(0x00017ac0, 0x6)
const AFX_MSGMAP* CBattlezDlgColors::GetMessageMap() const {
    return reinterpret_cast<const AFX_MSGMAP*>(&g_msgmap_CBattlezDlgColors); // msgmap global still a placeholder type
}

RVA(0x00017ae0, 0x20)
void CBattlezDlgColors::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    lpmis->itemWidth = 0xc8;
    lpmis->itemHeight = 0x1e;
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

// ---------------------------------------------------------------------------
// CBattlezDlg control accessors: switch(index) over a 4-entry control-ID table,
// each case tail-calling this->GetDlgItem(constID) (which returns the child
// CWnd*). Default (index>3) returns null. Four families, identical shape over
// different ID tables. The inline .rdata jump table reloc-masks.
//
// The dispatch + all four case bodies are byte-exact vs retail (verified by
// llvm-objdump base-vs-target); the ~70% plateau is the inline jump-table DATA
// region + its base reloc scored as mismatched (docs/patterns/jumptable-data-
// overlap.md). The result-var spelling forces the retail edx-index/xor-eax-eax
// dispatch (docs/patterns/switch-pointer-default-result-var.md).
// ---------------------------------------------------------------------------
// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015ac0, 0x46)
CWnd* CBattlezDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x500);
            break;
        case 1:
            result = GetDlgItem(0x50e);
            break;
        case 2:
            result = GetDlgItem(0x50f);
            break;
        case 3:
            result = GetDlgItem(0x510);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015b40, 0x46)
CWnd* CBattlezDlg::GetCtrlB(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x50a);
            break;
        case 1:
            result = GetDlgItem(0x50b);
            break;
        case 2:
            result = GetDlgItem(0x50c);
            break;
        case 3:
            result = GetDlgItem(0x50d);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015bc0, 0x46)
CWnd* CBattlezDlg::GetCtrlC(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x51e);
            break;
        case 1:
            result = GetDlgItem(0x520);
            break;
        case 2:
            result = GetDlgItem(0x521);
            break;
        case 3:
            result = GetDlgItem(0x522);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015c40, 0x46)
CWnd* CBattlezDlg::GetCtrlD(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x501);
            break;
        case 1:
            result = GetDlgItem(0x503);
            break;
        case 2:
            result = GetDlgItem(0x505);
            break;
        case 3:
            result = GetDlgItem(0x507);
            break;
    }
    return result;
}

RVA(0x00015cc0, 0x23)
i32 CBattlezDlg::SetCurSelA(i32 id, i32 sel) {
    CWnd* c = GetCtrlA(id);
    return ::SendMessageA(c->m_hWnd, 0x14e, sel, 0);
}

RVA(0x00015d00, 0x20)
i32 CBattlezDlg::Query015d00(i32 slot) {
    CWnd* c = GetCtrlA(slot);
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0);
}

RVA(0x00015d30, 0x21)
i32 CBattlezDlg::Query015d30(i32 id) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
}

RVA(0x00015d70, 0x24)
i32 CBattlezDlg::SetCurSelC(i32 id, i32 sel) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, 0x14e, sel - 1, 0);
}

RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetSlotValue(i32 index, i32 val) {
    m_slots->m_options[index].m_008 = val;
    return 1;
}

RVA(0x00017560, 0x28)
i32 CBattlezDlg::SaveOptionCombo0() {
    CWnd* c = GetCtrlC(0);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[0].m_comboSel = v;
    return v;
}
RVA(0x000175a0, 0x28)
i32 CBattlezDlg::SaveOptionCombo1() {
    CWnd* c = GetCtrlC(1);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[1].m_comboSel = v;
    return v;
}
RVA(0x000175e0, 0x28)
i32 CBattlezDlg::SaveOptionCombo2() {
    CWnd* c = GetCtrlC(2);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[2].m_comboSel = v;
    return v;
}
RVA(0x00017620, 0x28)
i32 CBattlezDlg::SaveOptionCombo3() {
    CWnd* c = GetCtrlC(3);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[3].m_comboSel = v;
    return v;
}

RVA(0x00015db0, 0x19)
void CBattlezDlg::SetCtrlBText(i32 index, const char* text) {
    CWnd* w = GetCtrlB(index);
    w->SetWindowTextA(text);
}

RVA(0x000173e0, 0x1)
void CBattlezDlg::RefreshOptionState() {}
RVA(0x00015de0, 0x5f)
void CBattlezDlg::ApplyOption0() {
    ToggleRow(0);
    RefreshOptionState();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015e60, 0x5f)
void CBattlezDlg::ApplyOption1() {
    ToggleRow(1);
    RefreshOptionState();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015ee0, 0x5f)
void CBattlezDlg::ApplyOption2() {
    ToggleRow(2);
    RefreshOptionState();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015f60, 0x5f)
void CBattlezDlg::ApplyOption3() {
    ToggleRow(3);
    RefreshOptionState();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

// -------------------------------------------------------------------------
// Per-color-slot apply handlers (0x16cd0/16dc0/16e90/16f60): pop the modal
// CBattlezDlgColors picker for slot N (a0=m_slots, a1=N), and on IDOK store the
// picked value (dlg.m_pickedColor) into slot N, refresh (RefreshOptionState), then invalidate the
// swatch control (0x501 + 2*N). The /GX EH frame unwinds the local dialog; the
// ctor/DoModal/dtor + SetSlotValue/RefreshOptionState/GetDlgItem chain + InvalidateRect
// import all reloc-mask. The four bodies differ only in N (the a1 arg, the
// SetSlotValue index, and the control ID).
// -------------------------------------------------------------------------
// The swatch refresh is the MFC inline CWnd::InvalidateRect member (afxwin2.inl:
// `::InvalidateRect(m_hWnd, lpRect, bErase)`), NOT the global import spelled on a
// hoisted handle. That is what fixes both halves of the old residual: the CWnd*
// is evaluated FIRST as the inline's `this` (retail pushes 0x501/call GetDlgItem
// before push 1/push 0), and because eax stays live as that `this`, the m_hWnd
// load lands in edx - `mov edx,[eax+0x1c]; push 1; push 0; push edx`, byte-exact.
// (Was @early-stop'd as an "eh-dtor vptr-restamp wall, not source-steerable" at
// 91.1%; that diagnosis was wrong - there is no restamp in this diff at all. The
// four bodies scoring an identical 91.1% was the shared ARG-ORDER residual, not a
// shared structural wall.)
RVA(0x00016cd0, 0x98)
void CBattlezDlg::ApplyColorSlot0() {
    CBattlezDlgColors dlg(m_slots, 0, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(0, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x501)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016dc0, 0x97)
void CBattlezDlg::ApplyColorSlot1() {
    CBattlezDlgColors dlg(m_slots, 1, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(1, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x503)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016e90, 0x98)
void CBattlezDlg::ApplyColorSlot2() {
    CBattlezDlgColors dlg(m_slots, 2, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(2, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x505)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016f60, 0x98)
void CBattlezDlg::ApplyColorSlot3() {
    CBattlezDlgColors dlg(m_slots, 3, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(3, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x507)->InvalidateRect(0, 1);
        }
    }
}

// CopyComboSelToChild (0x171b0): read the current selection text of the 0x4ff
// combo (CB_GETCURSEL via the ::SendMessageA global fn-ptr, then GetLBText into a
// local CString) and, if non-empty, push it into the combo's child edit
// (GetWindow(GW_CHILD) -> FromHandle -> SetWindowText) and latch m_68 = 0. /GX EH
// frame unwinds the local CString.
// @early-stop
// 96.8%: full logic byte-exact (combo GetCurSel via ::SendMessageA, GetLBText into the
// local CString, GetWindow(GW_CHILD)/FromHandle/SetWindowText, m_68 latch). Residual is the
// local CString's /GX unwind vptr/state ordering (same EH-restamp family), not steerable.
RVA(0x000171b0, 0xca)
void CBattlezDlg::CopyComboSelToChild() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return;
    }
    long sel = ::SendMessageA(combo->m_hWnd, 0x147, 0, 0);
    if (sel == -1) {
        return;
    }
    CString s;
    (static_cast<CComboBox*>(combo))->GetLBText(sel, s); // CComboBox::GetLBText @0x1ce7db
    if (s.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        if (child != 0) {
            child->SetWindowTextA(s);
            m_customNameFlag = 0;
        }
    }
}

// ReadCtrlBText (0x17340): read the `index` control's text into a local CString via
// GetCtrlB(index)->GetWindowText, then measure the resulting C-string. The /GX EH
// frame guards the half-built local CString.
// @early-stop
// trailing inlined-strlen block unmodeled (~69%): after GetWindowText, retail measures
// the filled buffer with an inline `repnz scas` (using edi as the scan pointer, hence an
// extra `push edi`) and DISCARDS the result. MSVC drops a discarded intrinsic strlen, so
// the scas can't be re-emitted without the original (unresolved) use of the length. The
// missing `push edi` shifts every [esp+N] reference by 4, depressing the byte score even
// though the CString ctor/dtor + GetCtrlB->GetWindowText + /GX frame are structurally exact.
RVA(0x00017340, 0x73)
void CBattlezDlg::ReadCtrlBText(i32 index) {
    CString s;
    GetCtrlB(index)->GetWindowText(s);
}

RVA(0x000160d0, 0xb)
i32 CBattlezDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return 1;
}

static __inline i32 GameRand() {
    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = static_cast<i32>(::timeGetTime());
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return (g_randSeed >> 0x10) & 0x7fff;
}

// @early-stop
// EH frame-size + regalloc wall (~84%). Complete correct reconstruction: the
// /GX EH frame, the 4-slot loop, the child->host rect map, the 3x inlined LCG
// colour, the inline CBrush ctor + inline vptr-stamp dtor chain, the rect-deflate
// and the NULL-guarded operator HBRUSH select all match by shape
// (llvm-objdump -dr). Residual is MSVC5 reserving a 0x70 frame vs ours
// (so dc/EH-state slots shift) and swapping the ecx/edx scratch regs in the
// strength-reduced *214013 LCG - not steerable from source. The CMultiStartDlg
// twin (no deflate) lives in MultiStartDlgRoster.cpp and reaches ~95%.
//
// The scratch object is a REAL MFC `CBrush` - proven by RTTI: the vtable its
// inline ctor stamps (0x1e8cf4) carries the COL .?AVCBrush@@, and the dtor chain
// stamps .?AVCGdiObject@@ (0x1e8cd4) / .?AVCObject@@ (0x1e8cb4). This INLINE
// default ctor is what forces this obj's ??_GCBrush/??_7CBrush COMDATs (pinned
// above). Check1be68c was CWnd::IsWindowEnabled (0x1be68c); the "FlashHost" view
// was the dialog itself (GetItem2c52 -> CBattlezDlg::GetCtrlD @0x15c40 via thunk
// 0x2c52). The g_p* "Win32 pointer table" externs were the plain dllimport IAT
// slots - `&::ClientToScreen` loads __imp__ClientToScreen@8 exactly as retail's
// cached `mov ebp/ebx, ds:[imp]`.
RVA(0x000160f0, 0x245)
void CBattlezDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == 0) {
            continue;
        }
        RECT rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, reinterpret_cast<LPPOINT>(&rc));
        cts(it->m_hWnd, reinterpret_cast<LPPOINT>(&rc) + 1);
        stc(m_hWnd, reinterpret_cast<LPPOINT>(&rc));
        stc(m_hWnd, reinterpret_cast<LPPOINT>(&rc) + 1);
        CBrush scratch;
        i32 color;
        if (it->IsWindowEnabled()) {
            GameRand();
            GameRand();
            i32 v = (GameRand() % 0xff) & 0xff;
            color = (v << 8 | v) << 8 | v;
        } else {
            color = 0x808080;
        }
        scratch.Attach(::CreateSolidBrush(color));
        rc.left += 2;
        rc.top += 2;
        rc.right -= 2;
        rc.bottom -= 2;
        ::FillRect(dc.m_hDC, &rc, scratch);
    }
}

RVA(0x00016570, 0x10)
void CBattlezDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

// CBattlezDlg::OnDrawItem (0x165a0): owner-draw the four team-color swatch static
// controls (0x501/0x503/0x505/0x507). Each maps to slot index 0..3; if the
// matching child window is enabled, fill the item rect with the slot's team color
// (the 17-entry palette-index -> COLORREF switch, inlined per control); a disabled
// child paints light gray. Always chains the base CWnd owner-draw default. /GX EH
// frame unwinds the CDC/CBrush locals.
//
// @early-stop
// jump-table-data split artifact (~71.5%; docs/patterns/jumptable-data-overlap.md) -
// the same wall that caps the sibling GetCtrlA/GetCtrlD in this TU at ~70%. COMPLETE +
// correct: prologue, /GX EH frame (0x18), outer nIDCtl dispatch (GetCtrlD(i)->
// IsWindowEnabled), the four inlined palette switches (index-declaration order - PROVEN
// right: the byte-identical CMultiStartDlg twin momentarily scored 98.7% off this exact
// shape when the delinker carved its full 0x5c0), and the whole CDC/CBrush/FillRect/
// Detach + base-OnDrawItem tail are all byte-exact. The residual is purely a delinker/
// scoring boundary: MSVC5 emits the 5 switch jump tables INLINE in .text (function =
// 0x5c0), but Ghidra carves the OnDrawItem boundary at 0x491 (code only) and splits the
// tables into separate switchdataD_00416a34.. data symbols, so objdiff compares our
// 0x5c0 base against a 0x491 target and scores the ~0x12f trailing jump-table bytes
// unmatched. Not source-steerable (a delinker fix that absorbs trailing in-range jump
// tables would lift this + GetCtrlA/D to ~98%).
RVA(0x000165a0, 0x5c0)
void CBattlezDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    i32 bDraw = 0;
    switch (nIDCtl) {
        case 0x501:
            if (GetCtrlD(0)->IsWindowEnabled()) {
                switch (m_slots->m_options[0].m_008) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x503:
            if (GetCtrlD(1)->IsWindowEnabled()) {
                switch (m_slots->m_options[1].m_008) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x505:
            if (GetCtrlD(2)->IsWindowEnabled()) {
                switch (m_slots->m_options[2].m_008) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x507:
            if (GetCtrlD(3)->IsWindowEnabled()) {
                switch (m_slots->m_options[3].m_008) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
    }
    if (bDraw) {
        CDC dc;
        dc.Attach(lpdis->hDC);
        CBrush brush(color);
        ::FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x00017440, 0x3)
i32 CBattlezDlg::UnusedMsgHandler() {
    return 0;
}

RVA(0x000172c0, 0x8)
void CBattlezDlg::OnActionBtn0() {
    ReadCtrlBText(0);
}
RVA(0x000172e0, 0x8)
void CBattlezDlg::OnActionBtn1() {
    ReadCtrlBText(1);
}
RVA(0x00017300, 0x8)
void CBattlezDlg::OnActionBtn2() {
    ReadCtrlBText(2);
}
RVA(0x00017320, 0x8)
void CBattlezDlg::OnActionBtn3() {
    ReadCtrlBText(3);
}

RVA(0x000174a0, 0x5)
void CBattlezDlg::OnOK() {
    CDialog::OnOK();
}

RVA(0x000174c0, 0x8)
void CBattlezDlg::OnStubBtn0() {
    StubBtnHandler(0);
}
RVA(0x000174e0, 0x8)
void CBattlezDlg::OnStubBtn1() {
    StubBtnHandler(1);
}
RVA(0x00017500, 0x8)
void CBattlezDlg::OnStubBtn2() {
    StubBtnHandler(2);
}
RVA(0x00017520, 0x8)
void CBattlezDlg::OnStubBtn3() {
    StubBtnHandler(3);
}
RVA(0x00017540, 0x3)
void CBattlezDlg::StubBtnHandler(i32) {}

RVA(0x00017d40, 0x8)
void CBattlezDlg::OnOkCommand() {
    OnOK();
}

// (The old "three DISTINCT image-holder classes" story here was RTTI-refuted: the
// vtables 0x1e8cb4/0x1e8cd4/0x1e8cf4/0x1ea2a4 carry RTTI COLs naming CObject/
// CGdiObject/CBrush/CRgn - the real MFC GDI family, whose ??1/??_G/??_7 COMDATs
// this TU (and the rect/creditz TUs for CRgn) first-instantiated. The ??_7 datums
// auto-name from config/vtable_names.csv when this obj emits the real COMDATs, and
// the function COMDATs are pinned by the RVA_COMPGEN block above.)
