// SFSelectDevice.h - the SFSelectDevice TU's exported globals/data.
#ifndef GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
#define GRUNTZ_GRUNTZ_SFSELECTDEVICE_H

#include <Win32.h>
#include <rva.h>

extern u16 g_idx_64da80;
extern DWORD g_ratingRaw_64da84;
extern i32 g_factoryRc_64da88;
extern char g_traceBuf_64da90[];
extern u16 g_remaining_64df98;
extern u32 g_id_64df9c;
extern DWORD g_sfVer;
extern u16 g_sfDeviceCount;
extern void* g_sfDll;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
extern SFMANL101API* g_sfDevice;
extern void* g_sfReady;
extern u8 g_ratings_64e0c0[];
#endif // GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
