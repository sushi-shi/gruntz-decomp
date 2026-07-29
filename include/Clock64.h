#ifndef GRUNTZ_CLOCK64_H
#define GRUNTZ_CLOCK64_H

#include <Ints.h>
#include <rva.h>

// A 64-bit engine clock / interval carried as its two DWORD halves.
//
// The timers are COMPARED and ACCUMULATED whole - retail loads them with the
// `mov eax,[x]; mov edx,[x+4]` pair a 64-bit operand needs - but the halves are
// also written INDIVIDUALLY (the ctors zero lo/lo/hi/hi interleaved across two
// timers), and they sit at 4-byte-aligned offsets inside classes whose sizes are
// not multiples of 8 (CGrunt 0x51c, CGruntzMgr's clock band, ...). A plain `i64`
// member would raise the class alignment to 8 and move every field after it, which
// is why these pairs used to be read through a pun.
//
// pack(4) removes that objection: the value keeps its 8 bytes at 4-byte alignment -
// exactly the layout retail has - and BOTH readings get a real name, so no reader
// needs a cast. x86 has no alignment requirement for a 64-bit load, and cl emits
// the same two-dword access either way.
#pragma pack(push, 4)
union Clock64 {
    i64 m_v; // the whole 64-bit value (every compare / accumulate reads this)
    struct {
        i32 m_lo;
        i32 m_hi;
    };
};
#pragma pack(pop)
SIZE(0x8);

#endif // GRUNTZ_CLOCK64_H
