// DataBuffer.h - the RezAlloc-backed serialized buffer holder at 0x150180..0x1503ed.
// Its retail name is CShadeTable (proven by the ShadeTableCache builders' call relocs -
// caller_callee), so the class + all its methods (Set/Reset/Free/LoadFromFile/
// LoadFromMem/ReadFrom/SaveToFile) live in the canonical <DDrawMgr/ShadeTableCache.h>.
// The former CDataBuffer placeholder view is GONE.
#ifndef GRUNTZ_DATABUFFER_H
#define GRUNTZ_DATABUFFER_H

#include <DDrawMgr/ShadeTableCache.h>

#endif // GRUNTZ_DATABUFFER_H
