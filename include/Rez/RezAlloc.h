#ifndef INCLUDE_REZ_REZALLOC_H
#define INCLUDE_REZ_REZALLOC_H
#include <Ints.h>

extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

#endif // INCLUDE_REZ_REZALLOC_H
