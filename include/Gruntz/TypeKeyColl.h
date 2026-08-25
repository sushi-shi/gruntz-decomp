#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Bute/ButeTree.h>
#include <Gruntz/TypeCollRuntime.h>

extern CTypeCollRuntime g_typeColl;

extern i32 g_typeCounter;

extern i32 g_variantOverrideCount;

void TmErrorHandler(char* prefix, i32 errNum);

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
