// WwdObjMgr.h - the WwdObjMgr TU's exported globals/data.
#ifndef GRUNTZ_WWD_WWDOBJMGR_H
#define GRUNTZ_WWD_WWDOBJMGR_H

#include <rva.h>

extern i32 g_wwdObjIdCounter;

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
// arg1 supplies its m_area (activation box), arg2 its m_switchRect - see the body.
i32 __stdcall BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj);

#endif // GRUNTZ_WWD_WWDOBJMGR_H
