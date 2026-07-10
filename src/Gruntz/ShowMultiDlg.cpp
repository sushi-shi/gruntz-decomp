// ShowMultiDlg.cpp - the ~CMultiStartDlg destructor COMDAT (0xb8960): retail
// emitted it beside its usage site (CNetMgrLite::ShowMultiStartDlg @0xb86c0
// STACK-constructs a CMultiStartDlg and runs it modally). Per the interval
// dossier (#4b) ShowMultiStartDlg itself lives in its home TU,
// src/Gruntz/Multi.cpp (with the CNetMgrLite host + cue views); this dtor COMDAT
// stays here per the dossier's "leave" verdict (COMDAT-at-usage - the dialog's
// own file is the 0xc16b0 MultiStartDlg.cpp TU).
//
// Built /GX (eh): the member dtors unwind inline. Field names are placeholders.
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <rva.h>

// ~CMultiStartDlg @0x0b8960 - destroy the CObList member m_74 then the CString
// member m_70, then chain the NAFXCW ~CDialog base dtor (all reloc-masked). /GX
// frame unwinds the half-torn object across the member dtors.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): same
// as ~CBattlezDlg - our polymorphic model emits one extra most-derived vptr re-stamp
// at entry that retail elided; the member/base teardown chain is otherwise byte-exact.
RVA(0x000b8960, 0x59)
CMultiStartDlg::~CMultiStartDlg() {}
