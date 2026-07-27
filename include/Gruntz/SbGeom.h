#ifndef GRUNTZ_GRUNTZ_SBGEOM_H
#define GRUNTZ_GRUNTZ_SBGEOM_H

#include <Mfc.h> // tagRECT (via afx.h -> windows.h), afx-first
#include <Ints.h>

// The status-bar geometry rect is Win32 RECT.
//
// It used to be TWO hand-rolled types: `SbiRect` (m_0/m_4/m_8/m_c, <Gruntz/StatusBarItem.h>)
// and `SbRect` (left/top/right/bottom, <Gruntz/SbRect.h>) - one shape under two names.
// They met field-for-field in every SBI setup body: CSBI_ImageSet::SetupImage assigned
// m_rect14.m_0/m_4/m_8/m_c straight from an SbRect's left/top/right/bottom, and
// StatusBarTabBuilders.cpp did the same from a plain RECT. Both are gone; RECT is the
// member type, the by-value parameter type and the local type everywhere.
//
// SbGeom is the construction seam the fold needs, and it is a FUNCTION, not a type: the
// setup virtuals take the rect BY VALUE and retail's callers build it directly in the
// outgoing argument frame (`sub esp,0x10; mov ecx,esp; mov [ecx],..; mov [ecx+4],..;
// mov [ecx+8],..; mov [ecx+0xc],..`; see the proof block over CStatusBarItem::Setup).
// A named local makes cl materialize the struct and copy it; an inline temporary makes
// it build in place. C++ has no `RECT(l,t,r,b)` expression for a POD aggregate, so the
// inline temporary comes from here.
inline RECT SbGeom(i32 l, i32 t, i32 r, i32 b) {
    RECT x;
    x.left = l;
    x.top = t;
    x.right = r;
    x.bottom = b;
    return x;
}

#endif // GRUNTZ_GRUNTZ_SBGEOM_H
