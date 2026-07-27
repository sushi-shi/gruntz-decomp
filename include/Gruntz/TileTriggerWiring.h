#ifndef GRUNTZ_TILETRIGGERWIRING_H
#define GRUNTZ_TILETRIGGERWIRING_H

#include <Ints.h>
#include <Win32.h> // tagRECT - CTrigParam is a RECT (see below)
#include <rva.h>

// CTrigParam IS a RECT. Two unrelated classes prove it at five offsets: this file's
// CTrigSourceRecord m_134/m_144/m_154 land exactly on CWwdGameObject's m_extent/m_area/
// m_switchRect, and CTrigRecordSub m_f0/m_100 on CAnimWorkerObj's m_switchRectA/B - both
// already documented there as REAL RECTs the registrar takes BY VALUE.
//
// It cannot simply BE tagRECT: AddLogicDefaults builds six zeroed blocks, and MSVC5 will
// not value-init a POD (`CTrigParam()` on a ctor-less struct copied garbage, 27%->76%),
// so the zeroing ctor is load-bearing. Hence a RECT-named struct that converts FROM a
// RECT - callers pass their rect members directly, no reinterpret at the boundary.
struct CTrigParam {
    i32 left, top, right, bottom;
    CTrigParam() : left(0), top(0), right(0), bottom(0) {} // VC5 won't value-init
    CTrigParam(const RECT& r) : left(r.left), top(r.top), right(r.right), bottom(r.bottom) {}
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
