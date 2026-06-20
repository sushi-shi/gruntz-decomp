// CMapStringToOb.h - compatibility shim: real MFC CMapStringToOb now comes from
// <Mfc.h> (via <afxcoll.h>). NOTE: MFC's POSITION is `__POSITION*`, not the old
// `typedef int POSITION` this shell used - callers of GetNextAssoc declare
// POSITION, so they pick up the real type.
#ifndef GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
#define GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
#include <Mfc.h>
#endif  // GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
