#ifndef GRUNTZ_TILETRIGGERWIRING_H
#define GRUNTZ_TILETRIGGERWIRING_H

#include <Ints.h>
#include <Win32.h>
#include <rva.h>

struct CTrigParam {
    i32 left, top, right, bottom;
    CTrigParam() : left(0), top(0), right(0), bottom(0) {}
    CTrigParam(const RECT& r) : left(r.left), top(r.top), right(r.right), bottom(r.bottom) {}
};
SIZE_UNKNOWN();

struct CTrigRecordSub {
    char _00[0xf0];
    CTrigParam m_f0;
    CTrigParam m_100;
};
SIZE_UNKNOWN();

struct CTrigSourceRecord {
    char _00[0x04];
    i32 m_4;
    char _08[0x64 - 0x08];
    CTrigParam m_64;
    char _74[0x7c - 0x74];
    CTrigRecordSub* m_7c;
    char _80[0x118 - 0x80];
    i32 m_118;
    char _11c[0x120 - 0x11c];
    i32 m_120;
    i32 m_124;
    i32 m_128;
    char _12c[0x134 - 0x12c];
    CTrigParam m_134;
    CTrigParam m_144;
    CTrigParam m_154;
    i32 m_164;
    i32 m_168;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TILETRIGGERWIRING_H
