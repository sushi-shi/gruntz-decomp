#ifndef GRUNTZ_GRUNTZ_UTILS_H
#define GRUNTZ_GRUNTZ_UTILS_H

#include <Mfc.h>

#include <Ints.h>

struct tagMODULEENTRY32;

void SetActiveAndFocus(HWND hWnd);
void SetTopmostStyle(HWND hWnd);
void ClearTopmostStyle(HWND hWnd);

i32 FileExists(const char* path);

int CheckHeap(BOOL bWalkIfErr);
void OutputHeapReturnValue(int val);
int HeapStats();

BOOL ExistProcess(const char* sExe, int thresh = 0, HANDLE* phProcess = NULL);
BOOL GetProcessModule(DWORD dwPID, DWORD dwModuleID, tagMODULEENTRY32* lpMe32, DWORD cbMe32);

CString TimeToString(DWORD dwTime);
void DissectTime(DWORD dwTime, int* pHour, int* pMin, int* pSec);
void TerminateString(char* text, i32 limit);
BOOL BlockScreenSaver(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // GRUNTZ_GRUNTZ_UTILS_H
