#ifndef GRUNTZ_MFCWIN_H
#define GRUNTZ_MFCWIN_H

// The <afxwin.h> surface. No TU includes an <afx*.h> directly - it comes through
// here, so the header's one ordering contract has exactly one owner instead of a
// copy at each include site.
//
// MFC ships its accessors as inline bodies guarded by _AFX_ENABLE_INLINES. The
// label pass and clangd cannot parse them (afxwin1.inl:1027 "a type specifier is
// required"), while MSVC 5.0 must see them to reproduce retail codegen - so the
// suppression is clang-only.
//
// <Mfc.h> MUST come first and is not optional: _AFX_ENABLE_INLINES is DEFINED by
// <afx.h>, so undefining it before afx.h is ever parsed is a no-op and clang
// then chokes on afxwin1.inl. Every hand-written site got this right by having
// pulled <Mfc.h> (directly or transitively) beforehand; the wrapper makes the
// dependency explicit instead of ambient.
#include <Mfc.h>

// GRUNTZ_MFC_NO_INLINES is the MSVC-side half of the same switch, for a TU whose
// OWN header pulls this file: <MfcNoInline.h> cannot reach those units, because the
// canonical include order parses the own header (and therefore <afxwin.h>) before
// the platform preludes, so its #undef lands after the .inl is already in. Retail
// proves the shape - `?Width@CRect@@QBEHXZ` and `?SetRect@CRect@@QAEXHHHH@Z` exist
// out of line and are called (font, gruntzmgr). Only afxwin1.inl is suppressed;
// afx.inl (CString &c.) is already parsed by <Mfc.h> above.
#if defined(__clang__) || defined(GRUNTZ_MFC_NO_INLINES)
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

#endif // GRUNTZ_MFCWIN_H
