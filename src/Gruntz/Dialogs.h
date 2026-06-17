// Dialogs.h - the MFC CDialog subclasses for the battle/multiplayer setup
// dialogs. Each ctor chains the MFC CDialog(UINT nIDTemplate, CWnd* pParent)
// base ctor (NAFXCW, reloc-masked), stores its own derived vftable, default-
// constructs any embedded MFC members (CString / CObList), and zero/inits the
// scalar members its ctor touches.
//
// THE HIERARCHY (RTTI-derived names; ImageBase 0x400000):
//   CDialog            MFC base.  ??0CDialog@@QAE@IPAVCWnd@@@Z @0x1ba8e9 (NAFXCW)
//   CBattlezDlg        Battlez (host/setup) dialog.  vftable @0x5e8bac
//   CBattlezDlgCustom  Battlez custom-rules dialog.  vftable @0x5e8ee4
//   CBattlezDlgColors  Battlez team-colors dialog.   vftable @0x5e8d94
//   CMultiStartDlg     Multiplayer start dialog.     vftable @0x5ea8ec
//   CCheckpointDlg     Checkpoint dialog.            vftable @0x5e9504
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code
// bytes are load-bearing (campaign doctrine). Each subclass is reconstructed
// with ONLY the members its ctor touches; the CDialog base is modeled as the
// 0x5c-byte slab the subclass members sit above (the subclass fields begin at
// +0x5c, so CDialog occupies +0x00..+0x5b incl. its vptr).
#ifndef SRC_GRUNTZ_DIALOGS_H
#define SRC_GRUNTZ_DIALOGS_H

// ---------------------------------------------------------------------------
// Minimal MFC base models. Only the exact mangled symbol + the calling
// convention/arg shape are load-bearing; the bodies live in NAFXCW and are
// never matched here (their `call rel32` displacements reloc-mask in objdiff).
// ---------------------------------------------------------------------------

// CWnd - referenced ONLY as the `CWnd*` 2nd arg of the CDialog ctor (so the
// base ctor mangles ??0CDialog@@QAE@IPAVCWnd@@@Z). Opaque; never dereferenced.
class CWnd;

// CString - the MFC string. Only its default ctor is touched (the embedded
// string members the dialog ctors construct in place). ??0CString@@QAE@XZ
// @0x1b9b93 (NAFXCW): m_pchData = *_afxEmptyString.
class CString {
public:
    CString();              // ??0CString@@QAE@XZ  @0x1b9b93
    ~CString();             // ??1CString@@QAE@XZ  @0x1b9cde (drives the EH unwind state)
    char *m_pchData;        // +0x00
};

// CObList - the MFC object list. Only the block-size ctor is touched (the
// embedded list CMultiStartDlg constructs with nBlockSize=0xa).
// ??0CObList@@QAE@H@Z @0x1b5d04 (NAFXCW). 0x1c bytes (vptr + 5 scalar fields).
class CObList {
public:
    CObList(int nBlockSize);    // ??0CObList@@QAE@H@Z  @0x1b5d04
    char m_body[0x1c];          // (incl. the implicit vptr at +0x00)
};

// CDialog - the MFC dialog base. The subclass ctors store their OWN vptr at
// [this] AFTER chaining this base ctor, so CDialog must be polymorphic (a
// virtual decl gives it a vptr at +0x00 and makes the derived vptr-store fall
// out of the ctor). It is padded to 0x5c bytes so the subclass members land at
// the offsets the disasm pins (+0x5c upward).
class CDialog {
public:
    CDialog(unsigned int nIDTemplate, CWnd *pParent);   // ??0CDialog@@QAE@IPAVCWnd@@@Z @0x1ba8e9
    virtual ~CDialog();                                 // (gives CDialog its vptr @+0x00)
    char m_body[0x5c - 4];                              // pad to 0x5c (vptr occupies +0x00)
};

// ---------------------------------------------------------------------------
// CBattlezDlg @ ctor 0x14b30 (vftable @0x5e8bac, __thiscall ret 8 = 2 args).
//   base CDialog(0xc0, pParent); m_5c = a0; CString @+0x6c; m_68 = 0.
// ---------------------------------------------------------------------------
class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(int a0, CWnd *pParent);

    int     m_5c;       // +0x5c  (= a0)
    char    m_pad60[8]; // +0x60
    int     m_68;       // +0x68  (= 0)
    CString m_6c;       // +0x6c  (default CString)
};

// ---------------------------------------------------------------------------
// CBattlezDlgCustom @ ctor 0x18030 (vftable @0x5e8ee4, __thiscall ret 4 = 1 arg).
//   base CDialog(0xc3, pParent); CString @+0x5c.
// ---------------------------------------------------------------------------
class CBattlezDlgCustom : public CDialog {
public:
    CBattlezDlgCustom(CWnd *pParent);

    CString m_5c;       // +0x5c  (default CString)
};

// ---------------------------------------------------------------------------
// CBattlezDlgColors @ ctor 0x17930 (vftable @0x5e8d94, __thiscall ret 0x10 = 4 args,
// NO EH frame - no embedded C++ object).
//   base CDialog(0xc2, pParent); m_5c = a0; m_60 = a1; m_64 = 0; m_68 = a2.
// ---------------------------------------------------------------------------
class CBattlezDlgColors : public CDialog {
public:
    CBattlezDlgColors(int a0, int a1, int a2, CWnd *pParent);

    int m_5c;   // +0x5c  (= a0)
    int m_60;   // +0x60  (= a1)
    int m_64;   // +0x64  (= 0)
    int m_68;   // +0x68  (= a2)
};

// ---------------------------------------------------------------------------
// CMultiStartDlg @ ctor 0xc1750 (vftable @0x5ea8ec, __thiscall ret 8 = 2 args).
//   base CDialog(0xc5, pParent); m_5c = a0; m_60 = 0; m_6c = 0;
//   CString @+0x70; CObList(0xa) @+0x74; then g_64bd5c = g_gameReg->m_2c.
// ---------------------------------------------------------------------------
class CMultiStartDlg : public CDialog {
public:
    CMultiStartDlg(int a0, CWnd *pParent);

    int     m_5c;       // +0x5c  (= a0)
    int     m_60;       // +0x60  (= 0)
    char    m_pad64[8]; // +0x64
    int     m_6c;       // +0x6c  (= 0)
    CString m_70;       // +0x70  (default CString)
    CObList m_74;       // +0x74  (CObList(0xa))
};

// ---------------------------------------------------------------------------
// CCheckpointDlg @ ctor 0x234a0 (vftable @0x5e9504, __thiscall ret 4 = 1 arg).
//   base CDialog(0xcd, pParent); no additional data members touched.
// ---------------------------------------------------------------------------
class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd *pParent);
};

#endif // SRC_GRUNTZ_DIALOGS_H
