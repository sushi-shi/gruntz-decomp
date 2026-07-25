// GameInfoString.h - the GameInfoString TU's external declarations.
#ifndef GRUNTZ_GAMEINFOSTRING_H
#define GRUNTZ_GAMEINFOSTRING_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

struct CGameInfoTime;

i32 ValidateGameTime(CGameInfoTime* t);                  // 0x118310
void SplitMillisToHMS(u32 n, u32* hh, u32* mm, u32* ss); // 0x119210

#endif // GRUNTZ_GAMEINFOSTRING_H
