#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Gruntz/ActRegistry.h>
#include <ZTools/ZDArray.h>

extern zDArray<CString> g_typeColl;

extern i32 g_typeCounter;

inline const CString& GetAnimationActName(i32 id) {
    return g_typeColl[id];
}

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
