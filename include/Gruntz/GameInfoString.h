#ifndef GRUNTZ_GAMEINFOSTRING_H
#define GRUNTZ_GAMEINFOSTRING_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

struct CGameInfoTime;

i32 ValidateGameTime(CGameInfoTime* t);
void SplitMillisToHMS(u32 n, u32* hh, u32* mm, u32* ss);

#endif // GRUNTZ_GAMEINFOSTRING_H
