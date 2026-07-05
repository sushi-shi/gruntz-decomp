// Dialogs.h - the MFC CDialog subclasses for the battle/multiplayer setup
// dialogs. Each ctor chains the MFC CDialog(UINT nIDTemplate, CWnd* pParent)
// base ctor (NAFXCW, reloc-masked), stores its own derived vftable, default-
// constructs any embedded MFC members (CString / CObList), and zero/inits the
// scalar members its ctor touches.
//
// THE HIERARCHY (RTTI-derived names; ImageBase 0x400000):
//   CDialog            MFC base.
//   CBattlezDlg        Battlez (host/setup) dialog.
//   CBattlezDlgCustom  Battlez custom-rules dialog.
//   CBattlezDlgColors  Battlez team-colors dialog.
//   CMultiStartDlg     Multiplayer start dialog.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code
// bytes are load-bearing (campaign doctrine). Each subclass is reconstructed
// with ONLY the members its ctor touches; the CDialog base is modeled as the
// 0x5c-byte slab the subclass members sit above (the subclass fields begin at
// +0x5c, so CDialog occupies +0x00..+0x5b incl. its vptr).
#ifndef SRC_GRUNTZ_DIALOGS_H
#define SRC_GRUNTZ_DIALOGS_H

#include <rva.h>
#include <Ints.h>

class CString; // full def via <Gruntz/String.h> below; needed by CWnd::GetWindowText
struct HWND__; // the opaque Win32 HWND (windows.h arrives with <Gruntz/String.h>)

// ---------------------------------------------------------------------------
// Minimal MFC base models. Only the exact mangled symbol + the calling
// convention/arg shape are load-bearing; the bodies live in NAFXCW and are
// never matched here (their `call rel32` displacements reloc-mask in objdiff).
// ---------------------------------------------------------------------------

// CWnd - the MFC window base. Referenced as the `CWnd*` 2nd arg of the CDialog
// ctor, as GetDlgItem's return, and as the holder of SetWindowTextA. The latter
// is a NAFXCW __thiscall method reached by call-rel32 (external/no-body so the
// displacement reloc-masks in objdiff); only the __thiscall arg shape is load-
// bearing here.
SIZE_UNKNOWN(CWnd);
class CWnd {
public:
    void SetWindowTextA(const char* lpszString);
    void EnableWindow(i32 bEnable);        // NAFXCW __thiscall (reloc-masked)
    void GetWindowTextA(CString& rString); // NAFXCW __thiscall (reloc-masked)
                                           // (GetWindowText macro-expands to this)
    // CComboBox::GetLBText(int, CString&) (0x1ce7db) - reloc-masked __thiscall.
    void GetLBText1ce7db(i32 nIndex, CString& rString);
    // NAFXCW static (HWND -> CWnd*), reached by call-rel32 (reloc-masks). MFC
    // declares it PASCAL (__stdcall).
    static CWnd* __stdcall FromHandle(HWND__* hWnd); // 0x1bb23a
    char m_pad00[0x1c];                              // +0x00
    HWND__* m_hWnd;                                  // +0x1c  wrapped window handle
};

// CString - the MFC string. Only its default ctor is touched (the embedded
// string members the dialog ctors construct in place).
// m_pchData = *_afxEmptyString.
#include <Gruntz/String.h>

// CObList - the MFC object list. Only the block-size ctor is touched (the
// embedded list CMultiStartDlg constructs with nBlockSize=0xa).
// 0x1c bytes (vptr + 5 scalar fields).
#include <Gruntz/ObList.h>

// CDialog - the MFC dialog base. The subclass ctors store their OWN vptr at
// [this] AFTER chaining this base ctor, so CDialog must be polymorphic (a
// virtual decl gives it a vptr at +0x00 and makes the derived vptr-store fall
// out of the ctor). It is padded to 0x5c bytes so the subclass members land at
// the offsets the disasm pins (+0x5c upward).
SIZE_UNKNOWN(CDialog);
class CDialog {
public:
    CDialog(u32 nIDTemplate, CWnd* pParent);
    virtual ~CDialog(); // (gives CDialog its vptr @+0x00)
    // GetDlgItem - the NAFXCW __thiscall resolver (control ID -> child CWnd*),
    // reached by call-rel32 (external/no-body so it reloc-masks). Inherited by the
    // dialog subclasses; their accessors tail-call it on `this`.
    CWnd* GetDlgItem(i32 nID) const;
    // DoModal - the NAFXCW modal loop (reloc-masked). Modeled non-virtual: retail
    // calls it directly on a known-type local (devirtualized), so a plain method
    // reproduces the `call rel32`.
    i32 DoModal();         // 0x1ba9d2
    char m_body[0x5c - 4]; // pad to 0x5c (vptr occupies +0x00)
};

// ---------------------------------------------------------------------------
// CBattlezDlg
//   base CDialog(0xc0, pParent); m_slots = a0; CString @+0x6c; m_68 = 0.
// ---------------------------------------------------------------------------
// A player-slot record in the battle dialog's slot array (CBattlezDlg::m_slots).
// 0x238 bytes/slot (the disasm scales index by 0x238 = lea x71, *8); only the
// +0x158 int field (set by SetSlotValue) and the size are load-bearing.
SIZE_UNKNOWN(CBattlezSlot);
struct CBattlezSlot {
    char pad0[0x158];
    i32 m_158; // +0x158  slot value (SetSlotValue)
    char pad15c[0x238 - 0x15c];
};

// The player-slot list CMultiStartDlg::BuildSlotList allocates (defined in
// Dialogs.cpp); only a pointer is needed here.
struct CMultiSlotList;

SIZE_UNKNOWN(CBattlezDlg);
class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(i32 a0, CWnd* pParent);
    ~CBattlezDlg() OVERRIDE; // 0x14c90 (destroy CString m_6c, chain ~CDialog)

    i32 m_slots;          // +0x5c  (= a0; the CBattlezSlot* slot-array base)
    char m_pad60[8];      // +0x60
    i32 m_customNameFlag; // +0x68  1 = custom level name typed, 0 = picked from list
    CString m_6c;         // +0x6c  (default CString)

    // Control accessors: switch(index) -> GetDlgItem(constID). Four families,
    // each over a 4-entry control-ID table.
    CWnd* GetCtrlA(i32 index);
    CWnd* GetCtrlB(i32 index);
    CWnd* GetCtrlC(i32 index);
    CWnd* GetCtrlD(i32 index);
    // Listbox cur-sel helpers over the GetCtrlA/GetCtrlC families (own
    // RVA-annotated bodies in Dialogs.cpp; 0x15cc0/d30/d70).
    i32 SetCurSelA(i32 id, i32 sel); // 0x015cc0  LB_SETCURSEL(GetCtrlA)
    i32 Query015d00(i32 slot);       // 0x015d00  LB_GETCURSEL(GetCtrlA)
    i32 Query015d30(i32 id);         // 0x015d30  LB_GETCURSEL(GetCtrlC) + 1
    i32 SetCurSelC(i32 id, i32 sel); // 0x015d70  LB_SETCURSEL(GetCtrlC, sel-1)
    // SetCtrlBText - GetCtrlB(index)->SetWindowTextA(text).
    void SetCtrlBText(i32 index, const char* text);
    // SetSlotValue - store val into slot[index].field@0x158; returns TRUE.
    i32 SetSlotValue(i32 index, i32 val);

    // ReadCtrlBText (0x17340): read control `index`'s text into a local CString
    // (GetCtrlB(index)->GetWindowText), then measure it. /GX EH frame unwinds the
    // half-built local CString.
    void ReadCtrlBText(i32 index);

    // ShowCustomDlg (0x17030): run a CBattlezDlgCustom modally; on IDOK, uppercase
    // its (non-empty) custom-name string and push it into the 0x4ff control's child.
    void ShowCustomDlg();

    // Slot/option helpers reached via ILT thunks (own CBattlezDlg methods, owned
    // as RVA stubs in src/Stub/ApiCallers.cpp; external/no-body here so the calls
    // reloc-mask). ToggleRow sets the active option N; Sub0173e0 refreshes.
    i32 ToggleRow(
        i32 option
    );                // 0x015fe0 (canonical ?ToggleRow@CBattlezDlg, homed in BattlezDlgRow.cpp)
    void Sub0173e0(); // 0x0173e0

    // The four per-option apply handlers (0x15de0/15e60/15ee0/15f60): set option N,
    // refresh, then enable IDOK when any of slots 1..3 is occupied.
    void ApplyOption0();
    void ApplyOption1();
    void ApplyOption2();
    void ApplyOption3();

    // Per-color-slot apply handlers (0x16cd0/16dc0/16e90/16f60): run the modal
    // CBattlezDlgColors picker for slot N, store its result into the slot, refresh,
    // and invalidate the swatch control.
    void ApplyColorSlot0();
    void ApplyColorSlot1();
    void ApplyColorSlot2();
    void ApplyColorSlot3();
    // Copy the 0x4ff combo's current selection text into its child edit (0x171b0).
    void CopyComboSelToChild();
};

// ---------------------------------------------------------------------------
// CBattlezDlgCustom
//   base CDialog(0xc3, pParent); CString @+0x5c.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CBattlezDlgCustom);
class CBattlezDlgCustom : public CDialog {
public:
    CBattlezDlgCustom(CWnd* pParent);
    ~CBattlezDlgCustom() OVERRIDE; // 0x17140 (destroy CString m_customName, chain ~CDialog)

    CString m_customName; // +0x5c  (default CString)
};

// ---------------------------------------------------------------------------
// CBattlezDlgColors (NO EH frame - no embedded C++ object).
//   base CDialog(0xc2, pParent); m_slots = a0; m_slotIndex = a1; m_pickedColor = 0; m_68 = a2.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CBattlezDlgColors);
class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(i32 a0, i32 a1, i32 a2, CWnd* pParent);
    // MFC GetMessageMap override: returns &CBattlezDlgColors::messageMap (modeled
    // non-virtual so it does not perturb the compiler-emitted vtable/ctor stamp;
    // only its 6 own bytes `mov eax,OFFSET msgmap; ret` are matched).
    const void* GetMessageMap();

    i32 m_slots;       // +0x5c  (= a0; the CBattlezSlot* slot-array base, from parent)
    i32 m_slotIndex;   // +0x60  (= a1; the slot being colored)
    i32 m_pickedColor; // +0x64  (= 0; the picked value read after DoModal)
    i32 m_68;          // +0x68  (= a2; always 0 at call sites; role unproven)
};

// ---------------------------------------------------------------------------
// CMultiStartDlg
//   base CDialog(0xc5, pParent); m_host = a0; m_slotList = 0; m_6c = 0;
//   CString @+0x70; CObList(0xa) @+0x74; then g_64bd5c = g_gameReg->m_curState.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMultiStartDlg);
class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(i32 a0, CWnd* pParent);
    ~CMultiStartDlg() OVERRIDE; // 0x0b8960 (destroy CObList m_74, CString m_70, chain ~CDialog)

    // Engine-label backlog stub (non-virtual placeholder; vtable-neutral).
    void InitPlayerSlots();

    // BuildSlotList (0xc1e60): allocate the player-slot list, derive the player
    // count from the game-registry snapshot, and seed the list.
    void BuildSlotList();
    // UpdateSlot (0xc1fd0): enable a dialog control by slot occupancy, then push
    // the current selection into the slot list.
    i32 UpdateSlot();
    // GetSlotIndex (0xc4b30): the current slot index (own method, reloc-masked).
    i32 GetSlotIndex();

    // SetupWorldCombo (0xc1840): fill the 0x4ff world combo from the GAME_MULTI
    // registry path, read-only its edit child, and subclass its wndproc.
    i32 SetupWorldCombo();
    // Sub_c3e30 (0xc3e30): post-setup self-call - commit the selected world/host
    // name into the CMulti game-state (defined in MultiStartDlgWorld.cpp).
    void Sub_c3e30();

    // --- Multiplayer roster refresh cluster (bodies in MultiStartDlgRoster.cpp) ---
    // The whole 0xc2000-0xc5000 method band drives ONE dialog (this class); the
    // former per-fn ApiCaller host views (SelHost/RosterHost/BattlezDlg_c4230/
    // EditAppendHost) were placeholder duplicates of CMultiStartDlg (proven: they
    // share GetCtrlA..D at 0xc26c0/40/c0/40 and self-call Drive @0xc40b0). The
    // net-game-config facets CNetGameDlg (NetGameDlg.cpp) / CNetConnCoord
    // (NetMgrMisc.cpp) are the SAME dialog - PROVEN and folded here (matcher-5):
    // their methods dispatch this class's per-slot accessors 0x1929/0x298c/0x1753/
    // 0x1159/GetCtrlD(0xc2840) on `this` and self-call Drive @0xc40b0. See the
    // "net-game-config facet" block below.
    void AppendChatLine(char* str); // 0xc2ce0  append a line to the 0x511 log edit
    i32 UpdatePlayers(i32 force);   // 0xc4230  refresh every player row from the roster
    void OnSlotSelect0();           // 0xc4ee0  cache slot 0's list cursel + re-drive
    void OnSlotSelect1();           // 0xc4f30  slot 1
    void OnSlotSelect2();           // 0xc4f80  slot 2 (list via GetCtrlC)
    void OnSlotSelect3();           // 0xc4fd0  slot 3
    void ToggleReady(i32 idx);      // 0xc50f0  toggle slot idx's ready flag from its box
    // Roster helpers (own methods reached through ILT thunks; reloc-masked, RVAs
    // live in sibling units / ApiCaller stubs).
    void Drive();                  // 0xc40b0  re-drive the connect state (body in NetMgrMisc.cpp)
    void Sync16db(i32);            // 0x016db
    void Sync227a();               // 0x0227a
    void Sync2c0c();               // 0x02c0c
    void Sync38d2();               // 0x038d2
    i32 LocalSlot2d4c();           // 0x02d4c  current local slot index
    CWnd* NameEdit298c(i32 idx);   // 0x0298c  name edit for slot idx
    CWnd* KindCombo1929(i32 idx);  // 0x01929  human/computer combo for slot idx
    CWnd* ReadyCheck1159(i32 idx); // 0x01159  ready checkbox for slot idx
    CWnd* ColourBtn1753(i32 idx);  // 0x01753  colour button for slot idx
    void SyncColour3a5d(i32 idx, i32 v); // 0x03a5d

    // --- Net-game-config facet (folded from CNetGameDlg / CNetConnCoord; bodies
    // stay in their own units NetGameDlg.cpp / NetMgrMisc.cpp so delinker packing
    // is undisturbed). PROVEN same class as CMultiStartDlg (matcher-5): every one
    // of these runs this class's per-slot accessors on `this`. ---
    // SyncChannelSlot (0xc2ab0; also reached via ILT thunk 0x3ffd as the
    // UpdatePlayers-loop per-row reconcile call) - reconcile one channel's player
    // slot after a join/leave. Was CNetGameDlg::UpdateSlot / the roster's
    // "SyncKind3ffd".
    void SyncChannelSlot(i32 ch);
    i32 EnableControls();     // 0xc4120  re-enable the four player-config controls (was CNetGameDlg)
    void VerifyCustomLevel(); // 0xc4c00  confirm every player has the same custom level (was CNetGameDlg)
    void ConnectStep();       // 0xc2a20  one connect step: reconcile slot 1 then Drive (was CNetConnCoord::Step)
    // this-side MFC forwarders the net facet reaches (CWnd/CDialog methods,
    // reloc-masked; CDialog is modeled without its CWnd base so declare here).
    void EnableWindow(i32 bEnable); // 0x1be6a7 (CWnd::EnableWindow on this)
    void OnOK();                    // 0x1bacc3 (CDialog::OnOK)

    // Per-slot control accessors: switch(index) over a 4-entry control-ID table,
    // each case returning this->GetDlgItem(constID). SAME shape as
    // CBattlezDlg::GetCtrlA..D (the inline .rdata jump table reloc-masks).
    CWnd* GetCtrlA(i32 index); // 0xc26c0
    CWnd* GetCtrlB(i32 index); // 0xc2740
    CWnd* GetCtrlC(i32 index); // 0xc27c0
    CWnd* GetCtrlD(i32 index); // 0xc2840

    // The GetSafeHwnd-style accessor the builders fold inline: (this != 0) ?
    // (handle @ +0x1c) : 0. Inline member so MSVC inlines it and keeps the null
    // test (matching retail's `test esi,esi; jne; xor eax,eax; mov eax,[esi+0x1c]`).
    i32 GetSafe1c() {
        return this == 0 ? 0 : *(i32*)((char*)this + 0x1c);
    }

    i32 m_host;                 // +0x5c  (= a0; the CNetDlgHost*/slot-array base)
    CMultiSlotList* m_slotList; // +0x60  (= 0; built in BuildSlotList)
    char m_pad64[8];            // +0x64
    i32 m_6c;                   // +0x6c  (= 0)
    CString m_70;               // +0x70  (default CString)
    CObList m_74;               // +0x74  (CObList(0xa))
};

// CCheckpointDlg - trivial CDialog (resource 0xcd); ctor only.
SIZE_UNKNOWN(CCheckpointDlg);
class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd* pParent);
    // MFC GetMessageMap override (see CBattlezDlgColors): returns the static map.
    const void* GetMessageMap();
};

#endif // SRC_GRUNTZ_DIALOGS_H
