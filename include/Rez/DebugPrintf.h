#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Ints.h>

void dprintf(char* fmt, ...);
void dprintf(i32 x, i32 y, char* fmt, ...);
void dprintf(u32 level, char* fmt, ...);
void dprintf(u32 level, i32 x, i32 y, char* fmt, ...);

void dgotoxy(i32 x, i32 y);

void dgotoxy(u32 level, i32 x, i32 y);
void dclrscr();
void dclrscr(u32 level);

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
