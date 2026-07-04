// String.h - compatibility shim: real MFC CString now comes from <Mfc.h>.
// Kept so existing `#include <Gruntz/String.h>` sites keep working; prefer
// <Mfc.h> in new code. Must precede any <windows.h>/DirectX (MFC rule).
#ifndef GRUNTZ_GRUNTZ_CSTRING_H
#define GRUNTZ_GRUNTZ_CSTRING_H
#include <Mfc.h>
#endif // GRUNTZ_GRUNTZ_CSTRING_H
