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

#include <Ints.h>

class CString; // full def via <Gruntz/CString.h> below; needed by CWnd::GetWindowText
struct HWND__; // the opaque Win32 HWND (windows.h arrives with <Gruntz/CString.h>)

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
class CWnd {
public:
    void SetWindowTextA(const char* lpszString);
    void EnableWindow(i32 bEnable);        // NAFXCW __thiscall (reloc-masked)
    void GetWindowTextA(CString& rString); // NAFXCW __thiscall (reloc-masked)
                                           // (GetWindowText macro-expands to this)
    // NAFXCW static (HWND -> CWnd*), reached by call-rel32 (reloc-masks). MFC
    // declares it PASCAL (__stdcall).
    static CWnd* __stdcall FromHandle(HWND__* hWnd); // 0x1bb23a
    char m_pad00[0x1c];                              // +0x00
    HWND__* m_hWnd;                                  // +0x1c  wrapped window handle
};

// CString - the MFC string. Only its default ctor is touched (the embedded
// string members the dialog ctors construct in place).
// m_pchData = *_afxEmptyString.
#include <Gruntz/CString.h>

// CObList - the MFC object list. Only the block-size ctor is touched (the
// embedded list CMultiStartDlg constructs with nBlockSize=0xa).
// 0x1c bytes (vptr + 5 scalar fields).
#include <Gruntz/CObList.h>

// CDialog - the MFC dialog base. The subclass ctors store their OWN vptr at
// [this] AFTER chaining this base ctor, so CDialog must be polymorphic (a
// virtual decl gives it a vptr at +0x00 and makes the derived vptr-store fall
// out of the ctor). It is padded to 0x5c bytes so the subclass members land at
// the offsets the disasm pins (+0x5c upward).
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
//   base CDialog(0xc0, pParent); m_5c = a0; CString @+0x6c; m_68 = 0.
// ---------------------------------------------------------------------------
// A player-slot record in the battle dialog's slot array (CBattlezDlg::m_5c).
// 0x238 bytes/slot (the disasm scales index by 0x238 = lea x71, *8); only the
// +0x158 int field (set by SetSlotValue) and the size are load-bearing.
struct CBattlezSlot {
    char pad[0x238];
};

class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(i32 a0, CWnd* pParent);
    ~CBattlezDlg(); // 0x14c90 (destroy CString m_6c, chain ~CDialog)

    i32 m_5c;        // +0x5c  (= a0; also reused as the CBattlezSlot* slot-array base)
    char m_pad60[8]; // +0x60
    i32 m_68;        // +0x68  (= 0)
    CString m_6c;    // +0x6c  (default CString)

    // Control accessors: switch(index) -> GetDlgItem(constID). Four families,
    // each over a 4-entry control-ID table.
    CWnd* GetCtrlA(i32 index);
    CWnd* GetCtrlB(i32 index);
    CWnd* GetCtrlC(i32 index);
    CWnd* GetCtrlD(i32 index);
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
    // reloc-mask). Sub015fe0 sets the active option N; Sub0173e0 refreshes; the
    // Query015d00(slot) probes whether a slot is occupied.
    void Sub015fe0(i32 option); // 0x015fe0
    void Sub0173e0();           // 0x0173e0
    i32 Query015d00(i32 slot);  // 0x015d00

    // The four per-option apply handlers (0x15de0/15e60/15ee0/15f60): set option N,
    // refresh, then enable IDOK when any of slots 1..3 is occupied.
    void ApplyOption0();
    void ApplyOption1();
    void ApplyOption2();
    void ApplyOption3();

    i32 winapi_016cd0_InvalidateRect();
    i32 winapi_016dc0_InvalidateRect();
    i32 winapi_016e90_InvalidateRect();
    i32 winapi_016f60_InvalidateRect();
    i32 winapi_0171b0_GetWindow_SendMessageA();
};

// ---------------------------------------------------------------------------
// CBattlezDlgCustom
//   base CDialog(0xc3, pParent); CString @+0x5c.
// ---------------------------------------------------------------------------
class CBattlezDlgCustom : public CDialog {
public:
    CBattlezDlgCustom(CWnd* pParent);
    ~CBattlezDlgCustom(); // 0x17140 (destroy CString m_5c, chain ~CDialog)

    CString m_5c; // +0x5c  (default CString)
};

// ---------------------------------------------------------------------------
// CBattlezDlgColors (NO EH frame - no embedded C++ object).
//   base CDialog(0xc2, pParent); m_5c = a0; m_60 = a1; m_64 = 0; m_68 = a2.
// ---------------------------------------------------------------------------
class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(i32 a0, i32 a1, i32 a2, CWnd* pParent);
    // MFC GetMessageMap override: returns &CBattlezDlgColors::messageMap (modeled
    // non-virtual so it does not perturb the compiler-emitted vtable/ctor stamp;
    // only its 6 own bytes `mov eax,OFFSET msgmap; ret` are matched).
    const void* GetMessageMap();

    i32 m_5c; // +0x5c  (= a0)
    i32 m_60; // +0x60  (= a1)
    i32 m_64; // +0x64  (= 0)
    i32 m_68; // +0x68  (= a2)
};

// ---------------------------------------------------------------------------
// CMultiStartDlg
//   base CDialog(0xc5, pParent); m_5c = a0; m_60 = 0; m_6c = 0;
//   CString @+0x70; CObList(0xa) @+0x74; then g_64bd5c = g_gameReg->m_2c.
// ---------------------------------------------------------------------------
class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(i32 a0, CWnd* pParent);
    ~CMultiStartDlg(); // 0x0b8960 (destroy CObList m_74, CString m_70, chain ~CDialog)

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
    // Sub_c3e30 (0xc3e30 via ILT thunk): post-setup self-call (reloc-masked).
    void Sub_c3e30();

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

    i32 m_5c;        // +0x5c  (= a0)
    i32 m_60;        // +0x60  (= 0)
    char m_pad64[8]; // +0x64
    i32 m_6c;        // +0x6c  (= 0)
    CString m_70;    // +0x70  (default CString)
    CObList m_74;    // +0x74  (CObList(0xa))
};

// CCheckpointDlg - trivial CDialog (resource 0xcd); ctor only.
class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd* pParent);
    // MFC GetMessageMap override (see CBattlezDlgColors): returns the static map.
    const void* GetMessageMap();
};

#endif // SRC_GRUNTZ_DIALOGS_H
