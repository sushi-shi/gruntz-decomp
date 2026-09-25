#ifndef GRUNTZ_GRUNTZ_UTILS_H
#define GRUNTZ_GRUNTZ_UTILS_H

#include <Ints.h>

#include <stddef.h>

struct tagMODULEENTRY32;

// Utils.cpp builds on <Win32.h> without STRICT, where HWND is void*.
void SetActiveAndFocus(void* hWnd);
void SetTopmostStyle(void* hWnd);
void ClearTopmostStyle(void* hWnd);

i32 FileExists(const char* path);

i32 CheckHeap(i32 bWalkIfErr);
void OutputHeapReturnValue(i32 val);
i32 HeapStats();

i32 ExistProcess(const char* sExe, i32 thresh = 0, void** phProcess = NULL);
i32 GetProcessModule(
    unsigned long dwPID,
    unsigned long dwModuleID,
    tagMODULEENTRY32* lpMe32,
    unsigned long cbMe32
);

#endif // GRUNTZ_GRUNTZ_UTILS_H
