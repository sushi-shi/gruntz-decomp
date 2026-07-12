// Dialogs.cpp - the Battlez multiplayer setup dialog retail .obj (one contiguous
// .text block, 0x14b30..0x18086): CBattlezDlg / CBattlezDlgCustom /
// CBattlezDlgColors and the small CImgHolder image-list helper hierarchy compiled
// alongside them. Each ctor chains the NAFXCW CDialog(UINT, CWnd*) base ctor
// (reloc-masked), stores its own derived vftable, default-constructs its embedded
// MFC members, and zero/inits the scalar members it touches.
//
// The other dialog classes that used to share this aggregate TU were split out
// (matcher-1 de-fragmentation) into their own single-region units: CCheckpointDlg
// -> CheckpointDlg.cpp; CMultiStartDlg core -> MultiStartDlg.cpp; CNetMgr::
// ShowMultiStartDlg + ~CMultiStartDlg -> ShowMultiDlg.cpp; CLatencyList::SelectItem
// -> SlotComboFill.cpp.
//
// Built /GX: three of the four ctors construct embedded MFC C++ objects (CString /
// CObList) and so carry an fs:0 EH frame (push -1 / push handler / mov fs:0,esp) to
// unwind a half-built object if a member ctor throws. (CBattlezDlgColors has NO
// embedded C++ member, so its body carries no EH frame even under /GX.)
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code bytes
// are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Globals.h>
#include <string.h> // inline strcmp (the empty-text WM_SETTEXT gate in the edit subclass)

// The game-manager view of the 0x64556c singleton (== g_gameReg); the battlez-setup
// option handlers persist per-slot dropdowns into its m_options array.
extern "C" CGruntzMgr* g_gameReg; // 0x64556c

// CImgHolder::DeleteImageList @0x1c6a5c IS MFC CImageList::DeleteImageList (afxcmn;
// ?DeleteImageList@CImageList@@QAEHXZ - returns BOOL); minimal local decl (links from MFC).
SIZE_UNKNOWN(CImageList);
class CImageList {
public:
    i32 DeleteImageList();
};

// ---------------------------------------------------------------------------
// 0x14b10 (first fn in the TU): a message-map handler that runs MFC's default
// processing - `return Default();` tail-jmps to CWnd::Default (reloc-masked).
RVA(0x00014b10, 0x5)
long CBattlezDlg::DoDefault() {
    return Default();
}

// ---------------------------------------------------------------------------
RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(i32 a0, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_slots = a0;
    m_customNameFlag = 0;
}

// ~CBattlezDlg @0x14c90 - destroy the CString member m_6c, then chain the NAFXCW
// ~CDialog base dtor (both reloc-masked). The /GX EH frame unwinds the half-torn
// object across the member dtor.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): the
// /GX frame + member ~CString + base ~CDialog chain are byte-exact, but our
// polymorphic model emits one extra `mov [esi],&??_7CBattlezDlg` re-stamp at entry
// that retail elided (its vtable already equals the base through the dtor). Not
// source-steerable; ~94.4%.
RVA(0x00014c90, 0x47)
CBattlezDlg::~CBattlezDlg() {}

// @confidence: low
// @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
// @early-stop
// BattlezSetupDlgInit @0x14d00 (RVA-homed from src/Stub/ApiCallers.cpp) - the Battlez
// multiplayer-setup dialog init (GAME code, 2664 B). Reads config via g_buteMgr
// ("Battlez_Setup" section: LastMaxGruntz%d / LastDiff%d / LastColour%d,
// DefaultMaxGruntz) + g_gameReg, populates the dialog controls (the "Computer
// (easy/normal/difficult)", "Human", "Player", "Serra", "Jebediah" combo/list
// strings) and drives them via the g_pSendMessageA / PTR_GetWindow / PTR_GetWindowLongA
// / PTR_SetWindowLongA function-pointer trampolines. Deferred to the leaf-first final
// sweep: a >512B body over ~20 CButeMgr/CString/CGameReg callees + a subclass window
// trampoline that must be modeled first; a partial under-counts AND diverges its
// regalloc, so the return-0 normalization artifact is kept per the >512B REVERT rule.
RVA(0x00014d00, 0xa68)
i32 __stdcall BattlezSetupDlgInit(i32) {
    return 0;
}

// ---------------------------------------------------------------------------
// Small CImageList-holder helper hierarchy compiled into the dialogs TU (its code
// lands between the CBattlezDlg ctor and CBattlezDlgCustom). A trivial CObject-ish
// grand-base (vtable @0x5e8cb4) whose out-of-line destructor is the standalone
// base-vptr stamp the binary keeps at 0x16410, and a derived holder (vtable
// @0x5e8cd4) that owns a CImageList its dtor frees. The class names are placeholders
// (only offsets + code bytes are load-bearing). Real-virtual model (twin of
// CHolder8c400 in BoundaryLowerDtors.cpp): NO manual `*(void**)this = &g_*Vtbl`
// store, NO g_*Vtbl extern - cl emits the implicit ??_7 stamps, which reloc-mask.
// ---------------------------------------------------------------------------

// CImgHolderBase - the polymorphic CObject-ish grand-base. Its EMPTY virtual dtor
// folds in as the LAST store of ~CImgHolder (retail's stamp-after-teardown order)
// and, being virtual, is ALSO emitted standalone: the 7-byte `mov [ecx],&??_7; ret`
// the binary keeps at 0x16410 (the ??_7CImgHolderBase reloc-masking the shared
// 0x5e8cb4 vtable). The non-trivial base subobject earns the leaf's /GX EH frame.
// Real-virtual model: cl emits the implicit grand-base re-stamp - no manual
// `*(void**)this = &g_*Vtbl`; RVA() pins the cl-emitted ??1 standalone to 0x16410.
// (Keeping the dtor INLINE is what makes ~CImgHolder fold its teardown; an
// out-of-line base dtor would instead emit a `call ??1CImgHolderBase` and break the
// fold.)
struct CImgHolderBase {
    RVA(0x00016410, 0x7)
    virtual ~CImgHolderBase() {}
};

// CImgHolder2 (RVA-homed from src/Stub/DiscoveredEh.cpp, was the synthetic CU55) - a
// SECOND byte-identical dialog image-holder: a derived holder whose /GX dtor frees an
// embedded CImageList (DeleteImageList 0x1c6a5c) between the implicit own vptr stamp
// (0x5e8cd4) and the folded CImgHolderBase re-stamp (0x5e8cb4). The RTTI name is
// unrecovered (Ghidra ClassUnknown_55), but the shape is exactly CImgHolder's, so it
// dissolves onto the same CImgHolderBase grand-base + shared CImageList shim above.
struct CImgHolder2 : CImgHolderBase {
    void DeleteImageList();          // 0x1c6a5c (NAFXCW CImageList::DeleteImageList)
    virtual ~CImgHolder2() OVERRIDE; // 0x016460
};
RVA(0x00016460, 0x46)
CImgHolder2::~CImgHolder2() {
    ((CImageList*)this)->DeleteImageList();
}

// CImgHolder - the derived holder. Its virtual dtor's implicit vptr stamp lands
// stamp-first, frees the embedded image list (CImageList::DeleteImageList @0x1c6a5c,
// reloc-masked), then the folded base teardown re-stamps the base vtable. The /GX EH
// frame guards the base teardown if DeleteImageList throws.
struct CImgHolder : CImgHolderBase {
    void DeleteImageList();         // 0x1c6a5c (NAFXCW CImageList::DeleteImageList, reloc-masked)
    virtual ~CImgHolder() OVERRIDE; // 0x016500
};

RVA(0x00016500, 0x46)
CImgHolder::~CImgHolder() {
    ((CImageList*)this)->DeleteImageList(); // 0x1c6a5c
}

// ---------------------------------------------------------------------------
RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}

// ~CBattlezDlgCustom @0x17140 - destroy the CString member m_customName, then chain the
// NAFXCW ~CDialog base dtor. /GX EH frame for the member unwind.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): same
// as ~CBattlezDlg - one extra most-derived vptr re-stamp our polymorphic model emits
// that retail elided; chain otherwise byte-exact. ~94.4%.
RVA(0x00017140, 0x47)
inline CBattlezDlgCustom::~CBattlezDlgCustom() {}

// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// The saved original window-proc of the edit child this subclass wraps (reloc-masked
// DATA; the installer snapshots it via GetWindowLongA). Twin of MultiStartDlgWorld's
// g_64bdc0.
DATA(0x00229d10)
extern i32 g_629d10;

// WndProc_15a10 (0x15a10) - the subclass window-proc installed on a read-only combo
// edit child (twin of WndProc_c1a10): swallow an empty WM_SETTEXT (keeps the shown
// selection text) and chain everything else to the saved original proc (g_629d10).
RVA(0x00015a10, 0x70)
extern "C" i32 CALLBACK WndProc_15a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {
        if (strcmp(g_emptyString, (const char*)lParam) == 0) {
            return 0;
        }
    }
    return CallWindowProcA((WNDPROC)g_629d10, hWnd, msg, wParam, lParam);
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
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): the
// inlined ~CBattlezDlgCustom teardown emits one extra `mov [esp+4],&??_7CBattlezDlgCustom`
// vptr re-stamp before ~CString that retail elided (its vtable already equals the base
// through the dtor). The ctor/DoModal/GetLength/MakeUpper/GetDlgItem/GetWindow/FromHandle/
// SetWindowText chain + the /GX frame are byte-exact; the restamp shifts the tail and its
// EH trylevel numbering (retail 0/1/2/-1 vs 0/1/-1) + the child!=0 branch polarity. Same
// wall the out-of-line ~CBattlezDlgCustom (0x17140) hits; not source-steerable. ~92.9%.
RVA(0x00017030, 0xc1)
void CBattlezDlg::ShowCustomDlg() {
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1) {
        if (dlg.m_customName.GetLength() != 0) {
            dlg.m_customName.MakeUpper();
            CWnd* item = GetDlgItem(0x4ff);
            CWnd* child = CWnd::FromHandle(GetWindow(item->m_hWnd, GW_CHILD));
            if (child != 0) {
                child->SetWindowTextA(dlg.m_customName);
                m_customNameFlag = 1;
            }
        }
    }
}

// ---------------------------------------------------------------------------
RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(i32 a0, i32 a1, i32 a2, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_slots = a0;
    m_slotIndex = a1;
    m_pickedColor = 0;
    m_68 = a2;
}

// The game's SendMessageA fn-ptr global (reloc-masked indirect call). Bound via
// DATA(0x006c44a4) later in this TU (CopyComboSelToChild); declared here so the
// earlier DoDataExchange can reach it.
extern long(WINAPI* g_pSendMessageA)(void* hWnd, unsigned msg, unsigned wp, long lp); // 0x6c44a4

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
    long(WINAPI * pSend)(void*, unsigned, unsigned, long);
    if (pDX->m_bSaveAndValidate) {
        CWnd* lb = GetDlgItem(0x515);
        pSend = g_pSendMessageA;
        long sel = pSend(lb->m_hWnd, 0x188, 0, 0);    // LB_GETCURSEL
        long data = pSend(lb->m_hWnd, 0x199, sel, 0); // LB_GETITEMDATA
        m_pickedColor = data;
        if (data >= 0x11) {
            m_pickedColor = 0x10;
        }
    } else {
        CWnd* lb = GetDlgItem(0x515);
        pSend = g_pSendMessageA;
        for (i32 i = 0; i < 0x11; i++) {
            i32 avail = 1;
            i32* rec = (i32*)(m_slots + 0x158); // -> slot[0].m_158 (color / m_170 occupancy)
            for (i32 j = 0; j < 4; j++) {
                if (rec[6] != 0 && rec[0] == i) { // occupied slot already using color i
                    avail = 0;
                }
                rec = (i32*)((char*)rec + 0x238);
            }
            if (avail) {
                long idx = pSend(lb->m_hWnd, 0x180, 0, (long)"Color"); // LB_ADDSTRING
                pSend(lb->m_hWnd, 0x19a, idx, i);                      // LB_SETITEMDATA
            }
        }
        pSend(lb->m_hWnd, 0x186, 0, 0); // LB_SETCURSEL
    }
}

// ---------------------------------------------------------------------------
RVA(0x00017ac0, 0x6)
const void* CBattlezDlgColors::GetMessageMap() {
    return &g_msgmap_CBattlezDlgColors;
}

// CBattlezDlgColors::OnMeasureItem (0x17ae0): the owner-draw colour-swatch list
// measures each item at 200x30, then chains the base CWnd::OnMeasureItem (0x1bbf18).
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

// Listbox helpers over the GetCtrlA/GetCtrlC control families (0x15cc0/d00/d30/
// d70): each resolves its control via a sibling GetCtrl (ecx=this is preserved
// across the call, so no reload is emitted - same shape as SetCtrlBText) then
// drives its listbox via SendMessageA (LB_GETCURSEL 0x147 / LB_SETCURSEL 0x14e).
// SetCurSelA/C set the selection; Query015d00/Query015d30 read it.
RVA(0x00015cc0, 0x23)
i32 CBattlezDlg::SetCurSelA(i32 id, i32 sel) {
    CWnd* c = GetCtrlA(id);
    return SendMessageA(c->m_hWnd, 0x14e, sel, 0);
}

RVA(0x00015d00, 0x20)
i32 CBattlezDlg::Query015d00(i32 slot) {
    CWnd* c = GetCtrlA(slot);
    return SendMessageA(c->m_hWnd, 0x147, 0, 0);
}

RVA(0x00015d30, 0x21)
i32 CBattlezDlg::Query015d30(i32 id) {
    CWnd* c = GetCtrlC(id);
    return SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
}

RVA(0x00015d70, 0x24)
i32 CBattlezDlg::SetCurSelC(i32 id, i32 sel) {
    CWnd* c = GetCtrlC(id);
    return SendMessageA(c->m_hWnd, 0x14e, sel - 1, 0);
}

// SetSlotValue (0x17460) - store val into slot[index].field@0x158; returns TRUE.
// Homed out-of-line (matcher-5).
RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetSlotValue(i32 index, i32 val) {
    ((CBattlezSlot*)m_slots)[index].m_158 = val;
    return 1;
}

// SaveOptionCombo0..3 (0x17560/175a0/175e0/17620): read control N's combobox
// selection (CB_GETCURSEL via GetCtrlC) and persist it (+1) into the game
// registry's option slot N. g_gameReg is the CGruntzMgr view of the 0x64556c
// singleton (folds the +0x150 options base + 0x228 into the mov displacement).
RVA(0x00017560, 0x28)
i32 CBattlezDlg::SaveOptionCombo0() {
    CWnd* c = GetCtrlC(0);
    i32 v = SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[0].m_comboSel = v;
    return v;
}
RVA(0x000175a0, 0x28)
i32 CBattlezDlg::SaveOptionCombo1() {
    CWnd* c = GetCtrlC(1);
    i32 v = SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[1].m_comboSel = v;
    return v;
}
RVA(0x000175e0, 0x28)
i32 CBattlezDlg::SaveOptionCombo2() {
    CWnd* c = GetCtrlC(2);
    i32 v = SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[2].m_comboSel = v;
    return v;
}
RVA(0x00017620, 0x28)
i32 CBattlezDlg::SaveOptionCombo3() {
    CWnd* c = GetCtrlC(3);
    i32 v = SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[3].m_comboSel = v;
    return v;
}

// SetCtrlBText - resolve control `index` via GetCtrlB (through the thunk) and
// push `text` into it via CWnd::SetWindowTextA (both NAFXCW, reloc-masked).
RVA(0x00015db0, 0x19)
void CBattlezDlg::SetCtrlBText(i32 index, const char* text) {
    CWnd* w = GetCtrlB(index);
    w->SetWindowTextA(text);
}

// Sub0173e0 (0x173e0) is the CBattlezDlg dialog-refresh helper the ApplyOption
// handlers call - a 1-byte `ret` no-op in retail (the "__fpclear" FID row at 0x173e0
// was a LOW false positive). Defined here so the ApplyOption calls bind to it.
RVA(0x000173e0, 0x1)
void CBattlezDlg::Sub0173e0() {}
//
// ApplyOption0..3 (0x15de0/15e60/15ee0/15f60): set the active option N, refresh
// the dialog, then enable IDOK (GetDlgItem(1)) when any of slots 1..3 is occupied
// (the short-circuit `||` reuses the failed-probe's zero in eax on the false path).
// The ToggleRow/Sub0173e0/Query015d00 calls reloc-mask (own CBattlezDlg methods
// homed as RVA stubs in src/Stub/ApiCallers.cpp).
RVA(0x00015de0, 0x5f)
void CBattlezDlg::ApplyOption0() {
    ToggleRow(0);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015e60, 0x5f)
void CBattlezDlg::ApplyOption1() {
    ToggleRow(1);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015ee0, 0x5f)
void CBattlezDlg::ApplyOption2() {
    ToggleRow(2);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015f60, 0x5f)
void CBattlezDlg::ApplyOption3() {
    ToggleRow(3);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

// -------------------------------------------------------------------------
// Per-color-slot apply handlers (0x16cd0/16dc0/16e90/16f60): pop the modal
// CBattlezDlgColors picker for slot N (a0=m_slots, a1=N), and on IDOK store the
// picked value (dlg.m_pickedColor) into slot N, refresh (Sub0173e0), then invalidate the
// swatch control (0x501 + 2*N). The /GX EH frame unwinds the local dialog; the
// ctor/DoModal/dtor + SetSlotValue/Sub0173e0/GetDlgItem chain + InvalidateRect
// import all reloc-mask. The four bodies differ only in N (the a1 arg, the
// SetSlotValue index, and the control ID).
// -------------------------------------------------------------------------
// @early-stop
// eh-dtor vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md):
// the /GX frame + CBattlezDlgColors-local ctor/DoModal/dtor + SetSlotValue/Sub0173e0/
// GetDlgItem chain + InvalidateRect import are byte-exact, but the local dtor's polymorphic
// teardown emits one extra vptr re-stamp retail elided (same wall the neighboring dialog
// dtors + ShowCustomDlg hit). All four bodies score an identical 91.1% -> shared structural
// residual, not the per-N push form. Not source-steerable.
// The InvalidateRect import (call ff 15 [ptr]); reloc-masked DIR32.
RVA(0x00016cd0, 0x98)
void CBattlezDlg::ApplyColorSlot0() {
    CBattlezDlgColors dlg(m_slots, 0, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(0, dlg.m_pickedColor)) {
            Sub0173e0();
            InvalidateRect(GetDlgItem(0x501)->m_hWnd, 0, 1);
        }
    }
}

// @early-stop
// eh-dtor vptr-restamp wall (see ApplyColorSlot0); 91.1%, logic byte-exact.
RVA(0x00016dc0, 0x97)
void CBattlezDlg::ApplyColorSlot1() {
    CBattlezDlgColors dlg(m_slots, 1, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(1, dlg.m_pickedColor)) {
            Sub0173e0();
            InvalidateRect(GetDlgItem(0x503)->m_hWnd, 0, 1);
        }
    }
}

// @early-stop
// eh-dtor vptr-restamp wall (see ApplyColorSlot0); 91.1%, logic byte-exact.
RVA(0x00016e90, 0x98)
void CBattlezDlg::ApplyColorSlot2() {
    CBattlezDlgColors dlg(m_slots, 2, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(2, dlg.m_pickedColor)) {
            Sub0173e0();
            InvalidateRect(GetDlgItem(0x505)->m_hWnd, 0, 1);
        }
    }
}

// @early-stop
// eh-dtor vptr-restamp wall (see ApplyColorSlot0); 91.1%, logic byte-exact.
RVA(0x00016f60, 0x98)
void CBattlezDlg::ApplyColorSlot3() {
    CBattlezDlgColors dlg(m_slots, 3, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(3, dlg.m_pickedColor)) {
            Sub0173e0();
            InvalidateRect(GetDlgItem(0x507)->m_hWnd, 0, 1);
        }
    }
}

// CopyComboSelToChild (0x171b0): read the current selection text of the 0x4ff
// combo (CB_GETCURSEL via the g_pSendMessageA global fn-ptr, then GetLBText into a
// local CString) and, if non-empty, push it into the combo's child edit
// (GetWindow(GW_CHILD) -> FromHandle -> SetWindowText) and latch m_68 = 0. /GX EH
// frame unwinds the local CString.
DATA(0x002c44a4)
extern long(WINAPI* g_pSendMessageA)(void* hWnd, unsigned msg, unsigned wp, long lp);
// @early-stop
// 96.8%: full logic byte-exact (combo GetCurSel via g_pSendMessageA, GetLBText into the
// local CString, GetWindow(GW_CHILD)/FromHandle/SetWindowText, m_68 latch). Residual is the
// local CString's /GX unwind vptr/state ordering (same EH-restamp family), not steerable.
RVA(0x000171b0, 0xca)
void CBattlezDlg::CopyComboSelToChild() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return;
    }
    long sel = g_pSendMessageA(combo->m_hWnd, 0x147, 0, 0);
    if (sel == -1) {
        return;
    }
    CString s;
    ((CComboBox*)combo)->GetLBText(sel, s); // CComboBox::GetLBText @0x1ce7db
    if (s.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        if (child != 0) {
            child->SetWindowTextA(s);
            m_customNameFlag = 0;
        }
    }
}

// CBattlezDlg::SetSlotValue (0x00017460) is now an inline member in the header.

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

// ---------------------------------------------------------------------------
// CBattlezDlg MFC message-map + dialog-frame handlers (referenced by the MFC
// message-map data, external to code here; each emits standalone at its RVA).
// ---------------------------------------------------------------------------
// OnInitDialog (vtable slot 49, 0x160d0): chain the base CDialog::OnInitDialog,
// then return TRUE (keep the dialog's default control focus).
RVA(0x000160d0, 0xb)
i32 CBattlezDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return 1;
}

// WM_MEASUREITEM (0x16570): forward (nIDCtl, lpmis) to the CWnd default owner-draw
// measure handler (no swatch sizing of its own, unlike CBattlezDlgColors').
RVA(0x00016570, 0x10)
void CBattlezDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

// The game's FillRect fn-ptr global (the .idata IAT slot, reloc-masked indirect
// call). Twin of FlashRect's g_pFillRect; DATA-bound here.
DATA(0x002c44e0)
extern int(WINAPI* g_pFillRectDlg)(HDC, const RECT*, HBRUSH);

// A minimal MFC CDC wrapper for the owner-draw swatch fill: vptr + the two device
// contexts. The dialog Attach()es the DRAWITEMSTRUCT hDC, fills, then Detach()es
// so ~CDC does not release the borrowed DC. All four members are NAFXCW bodies
// reached by call-rel32 (external/no-body -> reloc-masked).
SIZE_UNKNOWN(CDlgDC);
struct CDlgDC {
    char m_vfptr[4];    // +0x00  (CDC::CDC stamps the CObject vptr here; opaque slot)
    HDC m_hDC;          // +0x04
    HDC m_hAttribDC;    // +0x08
    i32 m_0c;           // +0x0c  (CDC::CDC zero-inits it; keeps the 0x10-byte size)
    CDlgDC();           // 0x1c563b  CDC::CDC
    void Attach(HDC h); // 0x1c5705  CDC::Attach
    void Detach();      // 0x1c573c  CDC::Detach
    ~CDlgDC();          // 0x1c5783  CDC::~CDC
};

// The owner-draw fill brush: a game GDI-object holder on the CImgHolderBase grand-
// base (twin of FlashRect's ImgHolder). Constructed from a COLORREF (0x1c6b18 ==
// CreateSolidBrush + attach; external/reloc-masked); its /GX inline virtual dtor
// stamps its own vtable, releases the brush (DeleteObject @0x1c6a5c), then folds in
// the CImgHolderBase re-stamp. SafeBrush NULL-guards the receiver (retail keeps the
// neg/sbb/and select even for the stack object).
struct CDrawBrush : CImgHolderBase {
    HBRUSH m_hObject;      // +0x04
    CDrawBrush(u32 color); // 0x1c6b18 (external)
    void Release1c6a5c();  // 0x1c6a5c (external)
    virtual ~CDrawBrush() OVERRIDE {
        Release1c6a5c();
    }
    HBRUSH SafeBrush() {
        return this ? m_hObject : (HBRUSH)0;
    }
};
SIZE_UNKNOWN(CDrawBrush);

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
                switch (((CBattlezSlot*)m_slots)[0].m_158) {
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
                switch (((CBattlezSlot*)m_slots)[1].m_158) {
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
                switch (((CBattlezSlot*)m_slots)[2].m_158) {
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
                switch (((CBattlezSlot*)m_slots)[3].m_158) {
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
        CDlgDC dc;
        dc.Attach(lpdis->hDC);
        CDrawBrush brush(color);
        g_pFillRectDlg(dc.m_hDC, &lpdis->rcItem, brush.SafeBrush());
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

// Unused message-map handler (0x17440): `xor eax,eax; ret` (returns 0).
RVA(0x00017440, 0x3)
i32 CBattlezDlg::UnusedMsgHandler() {
    return 0;
}

// Four button trampolines (0x172c0/0x172e0/0x17300/0x17320): each forwards its
// fixed index 0..3 to ReadCtrlBText (0x17340; the retail reloc target - the
// former "Sub01c340" placeholder misnamed 0x17340 as an external 0x1c340 body).
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

// OnOK (vtable slot 51, 0x174a0): forward to the base CDialog::OnOK (tail-jmp to
// ?OnOK@CDialog@@MAEXXZ @0x1bacc3, the NAFXCW protected virtual - reloc-masked/exempt).
RVA(0x000174a0, 0x5)
void CBattlezDlg::OnOK() {
    CDialog::OnOK();
}

// Four button trampolines (0x174c0/0x174e0/0x17500/0x17520): each forwards its
// fixed index 0..3 to the shared do-nothing StubBtnHandler.
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
// The shared button handler (0x17540): a no-op that just cleans its index arg (ret 4).
RVA(0x00017540, 0x3)
void CBattlezDlg::StubBtnHandler(i32) {}

// IDOK command trampoline (0x17d40): virtual-dispatch OnOK through this's vtable
// (slot 51 / +0xcc) - `mov eax,[ecx]; jmp [eax+0xcc]`.
RVA(0x00017d40, 0x8)
void CBattlezDlg::OnOkCommand() {
    OnOK();
}

SIZE_UNKNOWN(CImgHolderBase);
SIZE_UNKNOWN(CImgHolder);
// CImgHolder is the canonical owner of the shared image-holder vtable @0x1e8cd4 (the
// same vtable the inline-dtor twins CImgHolder2 here + CHolder8c400 in
// BoundaryLowerDtors alias); VTBL binds ??_7CImgHolder@@6B@ so its own-stamp is
// reloc-CORRECT. The base re-stamp -> ??_7CObject@@6B@ (0x1e8cb4) stays a compiler-
// model wall (cl emits ??_7CImgHolderBase for the inline-empty base, not CObject).
VTBL(CImgHolder, 0x001e8cd4);
SIZE_UNKNOWN(CImgHolder2);
RELOC_VTBL(
    CImgHolder2,
    0x001e8cd4
); // inline-dtor twin: aliases CImgHolder's vtable (dtor-stamp verified)
// The polymorphic grand-base's re-stamp target: cl emits ??_7CImgHolderBase@@6B@ for the
// empty inline-dtor base, but retail's base vtable IS the shared CObject vtable @0x1e8cb4
// (MSVC5 has no ICF; the base subobject's vtable equals CObject's). Bind the emitted
// ??_7CImgHolderBase symbol to that rva so the ~CImgHolderBase/~CImgHolder2/~CImgHolder
// base re-stamp DIR32s are reloc-CORRECT (was UNBOUND - the 0x1e8cb4 row is ??_7CObject).
RELOC_VTBL(CImgHolderBase, 0x001e8cb4);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
