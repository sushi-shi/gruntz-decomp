#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeTree.h>
#include <Wap32/ZVec.h>

extern zDArray<CString> g_typeColl;

extern i32 g_typeCounter;

extern i32 g_variantOverrideCount;

void TmErrorHandler(char* prefix, i32 errNum);

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
