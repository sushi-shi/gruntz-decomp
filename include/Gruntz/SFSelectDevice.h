// SFSelectDevice.h - the SFSelectDevice TU's exported globals/data.
#ifndef GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
#define GRUNTZ_GRUNTZ_SFSELECTDEVICE_H

#include <rva.h>

extern u16 g_idx_64da80;
extern u32 g_ratingRaw_64da84;
extern i32 g_factoryRc_64da88;
extern char g_traceBuf_64da90[];
extern char g_ratingBuf_64dbe0[];
extern u16 g_caps_64df30;
extern u32 g_capsFlags_64df36;
extern char g_capsName_64df46[];
extern u16 g_remaining_64df98;
extern u32 g_id_64df9c;
extern u32 g_sfVer;
extern u16 g_sfDeviceCount;
extern void* g_sfDll;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
extern SFMANL101API* g_sfDevice;
extern void* g_sfReady;
extern u8 g_ratings_64e0c0[];
#endif // GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
