#ifndef GRUNTZ_TILETRIGGERWIRING_H
#define GRUNTZ_TILETRIGGERWIRING_H

#include <Ints.h>
#include <rva.h>

struct CTrigParam {
    i32 m0, m4, m8, mc;
    CTrigParam() : m0(0), m4(0), m8(0), mc(0) {} // VC5 won't value-init -> zero by ctor
};
SIZE_UNKNOWN();

struct CTrigRecordSub {
    char _00[0xf0];
    CTrigParam m_f0;  // +0xf0
    CTrigParam m_100; // +0x100
};
SIZE_UNKNOWN();

struct CTrigSourceRecord {
    char _00[0x04];
    i32 m_4; // +0x04
    char _08[0x64 - 0x08];
    CTrigParam m_64; // +0x64
    char _74[0x7c - 0x74];
    CTrigRecordSub* m_7c; // +0x7c
    char _80[0x118 - 0x80];
    i32 m_118; // +0x118
    char _11c[0x120 - 0x11c];
    i32 m_120; // +0x120
    i32 m_124; // +0x124
    i32 m_128; // +0x128
    char _12c[0x134 - 0x12c];
    CTrigParam m_134; // +0x134
    CTrigParam m_144; // +0x144
    CTrigParam m_154; // +0x154
    i32 m_164;        // +0x164
    i32 m_168;        // +0x168
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TILETRIGGERWIRING_H
