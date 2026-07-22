#ifndef GRUNTZ_MAPLOGIC_H
#define GRUNTZ_MAPLOGIC_H

#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)

i32 MapSerializeCurve(CFileMemBase* ar, i32 mode, i32, i32); // 0x0ec230

#endif // GRUNTZ_MAPLOGIC_H
