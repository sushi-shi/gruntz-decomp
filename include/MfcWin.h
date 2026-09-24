#ifndef GRUNTZ_MFCWIN_H
#define GRUNTZ_MFCWIN_H

// Mfc.h must define _AFX_ENABLE_INLINES before this wrapper selects afxwin1.inl.
#include <Mfc.h>

#if defined(__clang__) || defined(GRUNTZ_MFC_NO_INLINES)
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

// Clang's header mirror qualifies VC5's implicit member-pointer addresses.
#ifdef __clang__
#define MFC_MESSAGE_MAP_CLASS(theClass) typedef theClass MfcMessageMapClass;
#else
#define MFC_MESSAGE_MAP_CLASS(theClass)
#endif

#endif // GRUNTZ_MFCWIN_H
