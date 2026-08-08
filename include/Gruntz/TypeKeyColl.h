#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Bute/ButeTree.h>
#include <Gruntz/TypeCollRuntime.h>

extern CTypeCollRuntime g_typeColl;

extern i32 g_typeCounter;

extern "C" i32 g_recCount23;

extern "C" i32 g_helperRefCount;

void TmErrorHandler(char* prefix, i32 errNum);

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
