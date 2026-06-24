// CAniPlayer.h - a timed cel-animation playback object (placeholder name; the
// RTTI class was not pinned - it is a non-virtual sprite-sequence player, NOT
// CAniCycle/CSimpleAnimation, whose vtables do not contain these methods).
//
// Two __thiscall methods (ascending RVA):
//   0x0e7980  Init  (seed the player from a sequence + rect + cel key; 14 args)
//   0x0e7b00  Tick  (timeGetTime-driven cel advance within the [+0x4c,+0x50] range)
//
// Layout recovered from the field stores (only OFFSETS + code bytes are
// load-bearing; names are placeholders):
//   +0x04 active flag        +0x24 sequence ptr (== +0x2c)
//   +0x0c int (arg2)         +0x28 frame countdown
//   +0x10 int (arg3)         +0x2c sequence ptr (== +0x24)
//   +0x14..0x20 rect (4 int) +0x30 current cel ptr
//   +0x34 cel-anim sub-obj   +0x38 current frame
//   +0x3c interval           +0x40 last time (timeGetTime)
//   +0x44 frame step         +0x48 wrap step
//   +0x4c low frame bound    +0x50 high frame bound
#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <Ints.h>

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c). Tick reaches the active
// render path through g->m_30->m_4->m_14; reloc-masked DIR32.
struct WwdGameReg {
    char _pad00[0x30];
    void* m_30; // +0x30
};
extern WwdGameReg* g_gameReg;

// The cel-animation sub-object held at CAniPlayer+0x34. Its +0x14 is the cel
// pointer table, indexed by frame; +0x64/+0x68 are the inclusive frame range.
struct AniCelTable {
    char _pad00[0x14];
    void** m_14; // +0x14  cel pointer table (m_14[frame] -> cel)
    char _pad18[0x64 - 0x18];
    i32 m_64; // +0x64  first frame
    i32 m_68; // +0x68  last frame
};

// One cel: +0x18/+0x1c are the x/y draw offsets the render call adds to the
// player's +0x14/+0x18 base.
struct AniCel {
    char _pad00[0x18];
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
};

// The sequence object passed to Init (held at +0x24/+0x2c). Its +0x10 holds a
// CMap (+0x10 into it) keyed by the cel key; lookup yields the AniCelTable.
struct AniSeq {
    char _pad00[0x10];
    void* m_10; // +0x10  -> map holder (+0x10 is the CMap)
};

class CAniPlayer {
public:
    i32 Init(
        AniSeq* seq,
        AniSeq* seq2,
        i32 a2,
        i32 a3,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 key,
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    );          // 0x0e7980
    i32 Tick(); // 0x0e7b00

    i32 m_00;          // +0x00
    i32 m_04;          // +0x04  active flag
    char _pad08[4];    // +0x08
    i32 m_0c;          // +0x0c
    i32 m_10;          // +0x10
    i32 m_rect[4];     // +0x14..0x20  rect (drawn through a common base)
    AniSeq* m_24;      // +0x24
    i32 m_28;          // +0x28
    AniSeq* m_2c;      // +0x2c
    AniCel* m_30;      // +0x30  current cel
    AniCelTable* m_34; // +0x34
    i32 m_38;          // +0x38  current frame
    i32 m_3c;          // +0x3c  interval
    i32 m_40;          // +0x40  last time
    i32 m_44;          // +0x44  step
    i32 m_48;          // +0x48  wrap step
    i32 m_4c;          // +0x4c  low bound
    i32 m_50;          // +0x50  high bound
};

#endif
