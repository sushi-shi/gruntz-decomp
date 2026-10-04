#ifndef GRUNTZ_INTS_H
#define GRUNTZ_INTS_H

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
#ifdef _MSC_VER
typedef __int64 i64;
typedef unsigned __int64 u64;
#else
typedef long long i64;
typedef unsigned long long u64;
#endif

typedef i32 b32;

#endif
