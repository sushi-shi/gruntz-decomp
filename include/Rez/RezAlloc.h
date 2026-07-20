#ifndef INCLUDE_REZ_REZALLOC_H
#define INCLUDE_REZ_REZALLOC_H
#include <Ints.h>

extern "C" void* RezAlloc(u32 size); // 0x1b9b46  operator new  (-> malloc 0x120b60)
extern "C" void RezFree(void* p);    // 0x1b9b82  operator delete (-> free 0x120c30)

#endif // INCLUDE_REZ_REZALLOC_H
