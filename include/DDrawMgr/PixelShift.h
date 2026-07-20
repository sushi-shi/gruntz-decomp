#ifndef INCLUDE_DDRAWMGR_PIXELSHIFT_H
#define INCLUDE_DDRAWMGR_PIXELSHIFT_H
#include <Ints.h>

extern i32 g_rUp;   // 0x683ea0  red   pack left-shift
extern i32 g_gUp;   // 0x683ea4  green pack left-shift
extern i32 g_bUp;   // 0x683ea8  blue  pack left-shift
extern i32 g_rDown; // 0x683eac  red   reduce right-shift
extern i32 g_gDown; // 0x683eb0  green reduce right-shift
extern i32 g_bDown; // 0x683eb4  blue  reduce right-shift

#endif // INCLUDE_DDRAWMGR_PIXELSHIFT_H
