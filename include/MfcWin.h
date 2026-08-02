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

#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

#endif // GRUNTZ_MFCWIN_H
