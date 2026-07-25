// WinAPICdRom.h - the WinAPICdRom.cpp TU's exported globals/functions.
#ifndef GRUNTZ_UTILS_WINAPICDROM_H
#define GRUNTZ_UTILS_WINAPICDROM_H

#include <Ints.h>

i32 IsGruntzCDInAnyDrive(); // 0x402540


// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
char GetGruntzDriveLetter();

#endif // GRUNTZ_UTILS_WINAPICDROM_H
