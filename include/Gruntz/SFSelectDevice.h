#ifndef GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
#define GRUNTZ_GRUNTZ_SFSELECTDEVICE_H

#include <rva.h>

#include <Win32.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(SoundFontDeviceRating)
    SF_DEVICE_RATING_UNUSABLE = 0x40
GZ_ENUM_CONST_END(SoundFontDeviceRating)

extern u16 g_idx_64da80;
extern DWORD g_ratingRaw_64da84;
extern i32 g_factoryRc_64da88;
extern char g_traceBuf_64da90[];
extern u16 g_remaining_64df98;
extern DWORD g_sfRouterId;
extern DWORD g_sfVer;
extern u16 g_sfDeviceCount;
extern HMODULE g_sfDll;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
extern SFMANL101API* g_sfDevice;
extern i32 g_sfReady;
extern u8 g_ratings_64e0c0[];
#endif // GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
