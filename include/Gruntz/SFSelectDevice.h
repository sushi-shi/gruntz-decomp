#ifndef GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
#define GRUNTZ_GRUNTZ_SFSELECTDEVICE_H

#include <rva.h>

#include <Win32.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(SoundFontDeviceRating)
    SF_DEVICE_RATING_UNUSABLE = 0x40
GZ_ENUM_CONST_END(SoundFontDeviceRating)

extern u16 g_sfDeviceIndex;
extern DWORD g_sfCandidateSampleBytes;
extern i32 g_sfManagerResult;
extern char g_sfTraceBuffer[];
extern u16 g_sfOpenAttemptsRemaining;
extern DWORD g_sfRouterId;
extern DWORD g_sfVer;
extern u16 g_sfDeviceCount;
extern HMODULE g_sfDll;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
extern SFMANL101API* g_sfDevice;
extern b32 g_sfReady;
extern u8 g_sfDeviceRatings[];
#endif // GRUNTZ_GRUNTZ_SFSELECTDEVICE_H
