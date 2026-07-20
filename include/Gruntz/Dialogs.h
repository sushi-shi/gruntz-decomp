#ifndef SRC_GRUNTZ_DIALOGS_H
#define SRC_GRUNTZ_DIALOGS_H

#include <Wap32/Object.h> // CObject (the single real MFC CObject)
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <rva.h>
#include <Ints.h>

class CString;               // full def via <Gruntz/String.h> below; needed by CWnd::GetWindowText
struct HWND__;               // the opaque Win32 HWND (windows.h arrives with <Gruntz/String.h>)
struct tagMEASUREITEMSTRUCT; // windows.h owner-draw measure (CWnd::OnMeasureItem arg)
struct tagDRAWITEMSTRUCT;    // windows.h owner-draw item    (CWnd::OnDrawItem arg)

#include <Net/NetLobby.h>
extern "C" i32 g_sharedFlag;
typedef LRESULT(WINAPI* WapSendMessageA)(HWND, UINT, WPARAM, LPARAM);
typedef HWND(WINAPI* WapGetWindow)(HWND, UINT);
extern "C" WapSendMessageA g_pSendMessageA; // 0x2c44a4
extern WapGetWindow g_pGetWindow;           // 0x2c44d8

#include <Gruntz/String.h>

#include <Gruntz/ObList.h>

SIZE(CDialog, 0x5c);

class CLatencyList;

SIZE_UNKNOWN(CBattlezDlg);
VTBL(CBattlezDlg, 0x001e8bac); // vtable_names -> code (RTTI game class)
class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(class CGruntzMgr* a0, CWnd* pParent);
    // NO DECLARED DESTRUCTOR - and that is a binary fact, not laziness. Retail's
    // ??1CBattlezDlg (0x14c90, 71 B) destroys the CString m_6c and chains ~CDialog with NO
    // `mov [esi],??_7CBattlezDlg` vptr re-stamp at entry. cl only emits that stamp for a
    // USER-WRITTEN dtor body (so a virtual call inside the body would dispatch to THIS
    // class); a compiler-generated one has no body and needs no stamp. Declaring it
    // out-of-line here cost the 6-byte stamp (94.4% wall) AND split the class in two: the
    // save-as dialog TU (GruntzMgr.cpp) had to keep a private twin with an implicit dtor,
    // because retail's CGruntzMgr::SaveGameAs INLINES the teardown at the stack local's
    // scope exit (`lea ecx,[esp+0x78]; call ~CString` / `lea ecx,[esp+0xc]; call ~CDialog`,
    // no `call ??1CBattlezDlg` anywhere) - which cl only does for an implicit dtor. Two
    // definitions, two byte-shapes, ONE mangled name: an ODR landmine that the implicit
    // dtor removes. The out-of-line COMDAT is still emitted (the vtable slot needs its
    // address) and is bound to 0x14c90 by @rva-symbol in src/Gruntz/Dialogs.cpp.
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (real MFC sig)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
    virtual i32 OnInitDialog() OVERRIDE;                      // slot 49  OnInitDialog (0x160d0)
    virtual void OnOK() OVERRIDE;                             // slot 51  OnOK (0x174a0)

    class CGruntzMgr* m_slots; // +0x5c  (= a0; the manager - its m_options[] is the slot array)
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
    // Persist per-slot dropdown selection (CB_GETCURSEL(GetCtrlC(N)) + 1) into the
    // game registry's option slot N (g_gameReg->m_options[N].m_comboSel).
    i32 SaveOptionCombo0(); // 0x017560
    i32 SaveOptionCombo1(); // 0x0175a0
    i32 SaveOptionCombo2(); // 0x0175e0
    i32 SaveOptionCombo3(); // 0x017620
    // SetCtrlBText - GetCtrlB(index)->SetWindowTextA(text).
    void SetCtrlBText(i32 index, const char* text);
    // SetSlotValue - store val into slot[index].field@0x158; returns TRUE. Out-of-line.
    i32 SetSlotValue(i32 index, i32 val); // 0x17460

    // ReadCtrlBText (0x17340): read control `index`'s text into a local CString
    // (GetCtrlB(index)->GetWindowText), then measure it. /GX EH frame unwinds the
    // half-built local CString.
    void ReadCtrlBText(i32 index);
    // FlashCtrlD (0x160f0; body in FlashRect.cpp): flash the four GetCtrlD swatch
    // controls with a random-gray (enabled) / fixed gray (disabled) CBrush fill.
    void FlashCtrlD();

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

    // --- MFC message-map handlers (referenced by the dialog's message-map data,
    //     external to code here; they emit standalone at their retail RVAs) ---
    // WM_MEASUREITEM forwarder (0x16570): pass (nIDCtl, lpmis) straight to the base
    // CWnd default (via the CWndOnMeasure shim); no owner-draw sizing of its own.
    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    // WM_DRAWITEM (0x165a0): owner-draw the four team-color swatch controls
    // (0x501/0x503/0x505/0x507). If the matching child is enabled, fill it with the
    // slot's team color (a 17-entry index->COLORREF switch); disabled -> light gray.
    // Then chain the base CWnd owner-draw default. /GX EH frame for the CDC/CBrush
    // locals.
    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);
    // 0x17440: an unused message-map handler - `xor eax,eax; ret` (returns 0).
    i32 UnusedMsgHandler();
    // 0x17d40: IDOK command trampoline - virtual-dispatch to this->OnOK
    // (vtable slot 51 / +0xcc): `mov eax,[ecx]; jmp [eax+0xcc]`.
    void OnOkCommand();
    // Four button trampolines (0x174c0/0x174e0/0x17500/0x17520): each forwards its
    // fixed index 0..3 to the (currently do-nothing) StubBtnHandler (0x17540).
    void OnStubBtn0();
    void OnStubBtn1();
    void OnStubBtn2();
    void OnStubBtn3();
    // 0x17540: the shared do-nothing button handler (`ret 4`; ignores its index arg).
    void StubBtnHandler(i32 index);
    // Four more button trampolines (0x172c0/0x172e0/0x17300/0x17320): each forwards
    // its fixed index 0..3 to ReadCtrlBText (0x17340, above) - the retail reloc target.
    void OnActionBtn0();
    void OnActionBtn1();
    void OnActionBtn2();
    void OnActionBtn3();
    // 0x14b10 (first fn in the TU): a message-map handler that just runs MFC's
    // default processing - `return Default();` tail-jmps to CWnd::Default.
    long DoDefault();
};

SIZE_UNKNOWN(CBattlezDlgCustom);
VTBL(CBattlezDlgCustom, 0x001e8ee4); // vtable_names -> code (RTTI game class)
class CBattlezDlgCustom : public CDialog {
public:
    CBattlezDlgCustom(CWnd* pParent);
    // No declared dtor - same fingerprint as CBattlezDlg above: retail's ??1CBattlezDlgCustom
    // (0x17140) destroys m_customName and chains ~CDialog with NO vptr re-stamp at entry, and
    // so does the copy CBattlezDlg::ShowCustomDlg inlines at its stack local. cl emits that
    // stamp for a user-written body only, so the dtor is compiler-generated. The COMDAT is
    // still emitted (the vtable slot takes its address) and bound by @rva-symbol in
    // src/Gruntz/Dialogs.cpp.
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (0x183d0; OrphanLeaves.cpp)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
    // 0x183f0 (OrphanLeaves.cpp): the custom-level listbox (0x516) confirm - the
    // message-map handler at messageMap+0x1c (LB dbl-click -> OnOK when selected).
    void PickIfSelected();

    CString m_customName; // +0x5c  (default CString)
};

SIZE_UNKNOWN(CBattlezDlgColors);
VTBL(CBattlezDlgColors, 0x001e8d94); // vtable_names -> code (RTTI game class)
class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(class CGruntzMgr* a0, i32 a1, i32 a2, CWnd* pParent);
    virtual ~CBattlezDlgColors() OVERRIDE; // slot 1
    // MFC GetMessageMap override: returns &CBattlezDlgColors::messageMap (modeled
    // non-virtual so it does not perturb the compiler-emitted vtable/ctor stamp;
    // only its 6 own bytes `mov eax,OFFSET msgmap; ret` are matched).
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (real MFC sig)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
    // WM_MEASUREITEM handler (0x17ae0): fixes the owner-draw swatch item size.
    void OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis);
    // DDX (0x179b0): save reads the 0x515 colour-list selection's item-data into
    // m_pickedColor (clamped to 0x10); load populates the list with the 17 colours
    // not already taken by an occupied player slot. Modeled non-virtual (like
    // GetMessageMap/OnMeasureItem) so it does not perturb the compiler-emitted vtable.

    class CGruntzMgr* m_slots; // +0x5c  (= a0; the manager, from parent)
    i32 m_slotIndex;   // +0x60  (= a1; the slot being colored)
    i32 m_pickedColor; // +0x64  (= 0; the picked value read after DoModal)
    i32 m_68;          // +0x68  (= a2; always 0 at call sites; role unproven)
};

SIZE_UNKNOWN(CMultiStartDlg);
VTBL(CMultiStartDlg, 0x001ea8ec); // vtable_names -> code (RTTI game class)
class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(class CGruntzMgr* a0, CWnd* pParent);
    // NO user-declared destructor: retail's ~CMultiStartDlg (0x0b8960) is the
    // COMPILER-GENERATED one (destroy CStringList m_74, CString m_70, chain
    // ~CDialog). cl 5.0 elides the most-derived vptr re-stamp in an IMPLICIT dtor
    // but always emits it for a user-declared one, even an empty `{}` (MEASURED
    // both ways, cl 5.0 /O2 /GX) - declaring it purely to hang an RVA() on was the
    // mis-model behind the old "unreachable restamp" wall. Still virtual (CDialog's
    // is). cl auto-emits the COMDAT in every using obj; it is @rva-symbol-bound in
    // src/Gruntz/Multi.cpp, whose CMulti::ShowMultiStartDlg (0xb86c0) stack-
    // constructs the dialog - which is exactly why retail emitted the COMDAT there.
    // docs/patterns/eh-dtor-vptr-restamp-presence.md
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (real MFC sig)
    virtual i32 DestroyWindow() OVERRIDE; // slot 24 (own override @0x00218a, origin CWnd)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
    virtual i32 OnInitDialog() OVERRIDE;                      // slot 49  OnInitDialog
    virtual void OnOK() OVERRIDE;                             // slot 51  OnOK

    // BuildSlotList (0xc1e60): allocate the player-slot list, derive the player
    // count from the game-registry snapshot, and seed the list. Returns 1 (tested
    // by DoDataExchange's load pass).
    i32 BuildSlotList();
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
    // SelHost/RosterHost/BattlezDlg_c4230/EditAppendHost are CMultiStartDlg methods
    // (they share GetCtrlA..D at 0xc26c0/40/c0/40 and self-call Drive @0xc40b0). The
    // net-game-config facets CNetGameDlg (NetGameDlg.cpp) / CNetConnCoord
    // (NetMgrMisc.cpp) are the SAME dialog:
    // their methods dispatch this class's per-slot accessors 0x1929/0x298c/0x1753/
    // 0x1159/GetCtrlD(0xc2840) on `this` and self-call Drive @0xc40b0. See the
    // "net-game-config facet" block below.
    // SetListCurSel (0xc2980): if list `id` resolves (GetCtrlC), set its LB_SETCURSEL to
    // wParam-1. A __thiscall member (ecx=this passthrough to GetCtrlC).
    void SetListCurSel(i32 id, i32 wParam);
    void AppendChatLine(char* str); // 0xc2ce0  append a line to the 0x511 log edit
    i32 UpdatePlayers(i32 force);   // 0xc4230  refresh every player row from the roster
    void OnSlotSelect0();           // 0xc4ee0  cache slot 0's list cursel + re-drive
    void OnSlotSelect1();           // 0xc4f30  slot 1
    void OnSlotSelect2();           // 0xc4f80  slot 2 (list via GetCtrlC)
    void OnSlotSelect3();           // 0xc4fd0  slot 3
    void ToggleReady(i32 idx);      // 0xc50f0  toggle slot idx's ready flag from its box

    // --- Per-slot colour handlers (bodies in MultiStartDlgColor.cpp) ---
    // Double-click a colour swatch (controls 0x501/0x503/0x505/0x507): if the slot is
    // pickable, run the modal CBattlezDlgColors picker, and on IDOK claim the colour
    // for the slot (SelectColor, the CNetSessHost +0x5c facet), re-drive, invalidate.
    void OnColorSlot0(); // 0xc3830
    void OnColorSlot1(); // 0xc3950
    void OnColorSlot2(); // 0xc3a70
    void OnColorSlot3(); // 0xc3b90
    // Double-click the world combo (0x4ff): run the modal CBattlezDlgCustom name dialog,
    // and on IDOK push the uppercased custom name into the combo's edit child + commit it
    // as the game's custom world name into the CMulti game-state.
    void OnCustomWorld(); // 0xc3cb0
    // Send the chat input (0x42d): prefix the local player's name, append to the log,
    // and broadcast the line to every peer, then clear the input.
    void OnChatSend(); // 0xc3f70
    // Commit the selected battle-latency option (list 0x527) into the CMulti session
    // config, or flag "none selected".
    void CommitLatencyOption(); // 0xc5020
    // Echo the current CmdDelay/ResendDelay session config to the chat log
    // (a diagnostic command; body in MultiStartDlgNet.cpp).
    void EchoLatencySettings(); // 0xc52f0
    // Roster helpers (own methods reached through ILT thunks; reloc-masked, RVAs
    // live in sibling units / ApiCaller stubs).
    void Drive();           // 0xc40b0  re-drive the connect state (body in NetMgrMisc.cpp)
    i32 Sync16db(i32);      // 0x016db  (DoDataExchange tests the result)
    void Sync227a();        // 0x0227a
    i32 UpdateColorItems(); // 0xc1aa0 (color-item refresh; via 0x02c0c thunk; ex m4::MultiColorDlg)
    i32 Sync38d2();         // 0x038d2  (DoDataExchange tests the result)
    i32 LocalSlot2d4c();    // 0x02d4c  current local slot index
    CWnd* NameEdit298c(i32 idx);         // 0x0298c  name edit for slot idx
    CWnd* KindCombo1929(i32 idx);        // 0x01929  human/computer combo for slot idx
    CWnd* ReadyCheck1159(i32 idx);       // 0x01159  ready checkbox for slot idx
    CWnd* ColourBtn1753(i32 idx);        // 0x01753  colour button for slot idx
    void SyncColour3a5d(i32 idx, i32 v); // 0x03a5d

    // --- Net-game-config facet (bodies stay in their own units NetGameDlg.cpp /
    // NetMgrMisc.cpp so delinker packing
    // is undisturbed). PROVEN same class as CMultiStartDlg (matcher-5): every one
    // of these runs this class's per-slot accessors on `this`. ---
    // SyncChannelSlot (0xc2ab0; also reached via ILT thunk 0x3ffd as the
    // UpdatePlayers-loop per-row reconcile call) - reconcile one channel's player
    // slot after a join/leave. Was CNetGameDlg::UpdateSlot / the roster's
    // "SyncKind3ffd".
    void SyncChannelSlot(i32 ch);
    i32 EnableControls(); // 0xc4120  re-enable the four player-config controls
    void
    VerifyCustomLevel(); // 0xc4c00  confirm every player has the same custom level
    void
    ConnectStep(); // 0xc2a20  one connect step: reconcile slot 1 then Drive
    // Message-map handlers: reconcile channel 2 / 3 (SyncChannelSlot) then re-drive
    // the connect state. Twins of ConnectStep (channel 1); PROVEN CMultiStartDlg (they
    // call this->SyncChannelSlot(0xc2ab0) + this->Drive(0xc40b0)). Bodies in
    // NetMgrMisc.cpp.
    void Method_c2a50(); // 0xc2a50  reconcile channel 2 then Drive
    void Method_c2a80(); // 0xc2a80  reconcile channel 3 then Drive
    // this-side MFC forwarders the net facet reaches (CWnd/CDialog methods,
    // reloc-masked; CDialog is modeled without its CWnd base so declare here).
    void EnableWindow(i32 bEnable); // 0x1be6a7 (CWnd::EnableWindow on this)
    // (CDialog::OnOK @0x1bacc3 is the inherited protected virtual slot-51 above;
    //  VerifyCustomLevel reaches it as CDialog::OnOK() - no separate non-virtual decl.)
    void M1bab37(i32); // 0x1bab37 (NAFXCW forwarder; the Watchdog abort/reshow)

    // Watchdog (0xc46b0): the per-timer multiplayer-session watchdog (body in
    // NetGameDlgWatch.cpp) - refresh the roster, advance the blink counters, then walk
    // the CMulti session status flags and tear down on any terminal condition.
    void Watchdog();

    // Per-slot control accessors: switch(index) over a 4-entry control-ID table,
    // each case returning this->GetDlgItem(constID). SAME shape as
    // CBattlezDlg::GetCtrlA..D (the inline .rdata jump table reloc-masks).
    CWnd* GetCtrlA(i32 index); // 0xc26c0
    CWnd* GetCtrlB(i32 index); // 0xc2740
    CWnd* GetCtrlC(i32 index); // 0xc27c0
    CWnd* GetCtrlD(i32 index); // 0xc2840
    i32 GetComboSelC(i32 id);  // 0xc2940  GetCtrlC combo cur-sel + 1 (-1 if missing)
    // WM_DRAWITEM (0xc3100): owner-draw the four team-color swatch controls
    // (0x501/0x503/0x505/0x507) - the exact twin of CBattlezDlg::OnDrawItem, over the
    // m_host slot array's per-slot color index. Chains the base CWnd owner-draw default.
    void OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis);
    // FlashCtrlD (0xc2e20; body in FlashRect.cpp): flash the four GetCtrlD swatch
    // controls - the CBattlezDlg::FlashCtrlD twin, no rect-deflate, returns 1.
    i32 FlashCtrlD();

    // The GetSafeHwnd-style accessor the builders fold inline (CWnd::m_hWnd @+0x1c): (this != 0) ?
    // (handle @ +0x1c) : 0. Inline member so MSVC inlines it and keeps the null
    // test (matching retail's `test esi,esi; jne; xor eax,eax; mov eax,[esi+0x1c]`).
    i32 GetSafe1c() {
        return this == 0 ? 0 : reinterpret_cast<i32>(m_hWnd);
    }

    class CGruntzMgr* m_host; // +0x5c  (= a0) the game manager (ex "heterogeneous handle":
                              //         the CNetDlgHost/CMultiSlot[]/CFocusSlot[] views were all
                              //         MGR facets - m_options[] slots + m_symParser @+0x34)
                              //        slot-array base (Dialogs.cpp/MultiStartDlgRoster.cpp) AND
                              //        the CNetDlgHost host-facet (FindOptionsSlot @0x92e80 +
                              //        m_registry @+0x34) - each site casts to the type it needs.
    CLatencyList* m_slotList; // +0x60  (= 0; built in BuildSlotList)
    char m_pad64[8];          // +0x64
    i32 m_6c;                 // +0x6c  (= 0)
    CString m_70;             // +0x70  (default CString)
    CStringList m_74;         // +0x74  (CStringList(0xa); ctor 0x1b5d04 / dtor 0x1b5d78)
};

SIZE_UNKNOWN(CCheckpointDlg);
VTBL(CCheckpointDlg, 0x001e9504); // vtable_names -> code (RTTI game class)
class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd* pParent);
    // No declared dtor - same reason as CBattlezDlg above. The declared-only
    // `virtual ~CCheckpointDlg()` that used to sit here had a body in NO obj (a guaranteed
    // unresolved external) and would have forced a `call ??1CCheckpointDlg` at the stack
    // local in CGruntzMgr::OnCheckpointReached, where retail inlines the teardown
    // (`call ??1CDialog` @0x1ba51d, nothing else). Implicit is both correct and linkable.
    // MFC GetMessageMap override (see CBattlezDlgColors): returns the static map.
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (real MFC sig)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
    // DDX (0x23520): on load, cache this dialog's HWND into NetLobby::g_curDlg and
    // clear the "disable prompts" checkbox (control 0x53a, BM_SETCHECK 0). Modeled
    // non-virtual (like CBattlezDlgColors::DoDataExchange) so it does not perturb
    // the compiler-emitted vtable.
    // Checkbox handler (0x23590): mirror control 0x53a into m_isCheckpointPrompts.
    void OnToggleCheckpointPrompts();
};

SIZE_UNKNOWN(CMultiHelpDlg);
class CMultiHelpDlg : public CDialog {
public:
    virtual ~CMultiHelpDlg() OVERRIDE;                        // slot 1
    virtual const AFX_MSGMAP* GetMessageMap() const OVERRIDE; // slot 12 (real MFC sig)
    virtual void DoDataExchange(CDataExchange* pDX) OVERRIDE; // slot 35
};
VTBL(CMultiHelpDlg, 0x001ea474);

#endif // SRC_GRUNTZ_DIALOGS_H
