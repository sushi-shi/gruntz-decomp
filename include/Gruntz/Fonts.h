#ifndef GRUNTZ_GRUNTZ_FONTS_H
#define GRUNTZ_GRUNTZ_FONTS_H

#include <Ints.h>

i32 InitializeFonts(); // 0x115810

extern i32 g_loadedFlag;
i32 FreeFontsMemory(); // 0x1158f0  release the four font objects' buffers

#endif // GRUNTZ_GRUNTZ_FONTS_H
