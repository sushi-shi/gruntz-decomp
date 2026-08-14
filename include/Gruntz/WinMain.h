#ifndef GRUNTZ_WINMAIN_H
#define GRUNTZ_WINMAIN_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

i32 FindProcessByName(const char* name, i32 wantCount, void** pHandleOut);
i32 StartUpPrompt(HWND__* parent);
void ActiveWait(u32 milliseconds);

#endif // GRUNTZ_WINMAIN_H
