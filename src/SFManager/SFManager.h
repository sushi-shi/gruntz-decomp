// SFManager.h - the engine SFMAN32.DLL SoundFont manager wrapper.
// SelectBestDevice enumerates SoundFont devices and picks the best one by
// a rating/heuristic. This is a free function (not a method).
#ifndef SFMANAGER_H
#define SFMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

extern "C" {
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *lpLibFileName);
__declspec(dllimport) void *__stdcall GetProcAddress(void *hModule, const char *lpProcName);
__declspec(dllimport) int   __stdcall FreeLibrary(void *hLibModule);
}

#ifdef __cplusplus
}
#endif

// The engine's string formatting function (__cdecl).
extern "C" int __cdecl EngFormat(char *dest, const char *fmt, ...);

// ---------------------------------------------------------------------------
// SelectBestDevice - enumerates SoundFont devices through SFMAN32.DLL, selects
// the best one by a rating heuristic, and stores the results in module-level
// globals. Returns nonzero on success.
// ---------------------------------------------------------------------------
int SelectBestDevice();

#endif // SFMANAGER_H
