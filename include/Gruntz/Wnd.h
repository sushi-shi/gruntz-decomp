// Wnd.h - hands the Win32/dialog TUs the REAL MFC CCmdTarget/CWnd from <afxwin.h>.
// It is now a one-line shim; there is nothing left to hand-roll.
#ifndef GRUNTZ_GRUNTZ_CWND_H
#define GRUNTZ_GRUNTZ_CWND_H

#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h> // CObject (the real MFC one; pulls <Mfc.h> -> afx.h FIRST)
// The REAL MFC CCmdTarget / CWnd. This header used to hand-roll BOTH on CObject with ~26
// CtVslN/WndVslN placeholder virtuals purely to anchor MFC's vtable slot order - the same
// second copy of the lie <Gruntz/Dialogs.h> carried. Every member the consumers actually
// use (CWnd(), CreateEx, SetFocus, EnableWindow, FromHandle, m_hWnd) is a real MFC member,
// so the real classes are a drop-in. afx.h arrives first via Wap32/Object.h, so there is no
// windows.h-first C1189 (and <Win32.h> is no longer needed - afx brings windows.h itself).
// afxwin*.inl is skipped for the clang LABEL step only (implicit-int CMenu::operator==);
// without it the label pass emits no IR and the whole TU vanishes from symbol_names.csv.
// See docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

#endif // GRUNTZ_GRUNTZ_CWND_H
