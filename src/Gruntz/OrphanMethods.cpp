// OrphanMethods.cpp - moderate orphan-COMDAT methods with no recoverable owning
// class. Each is modeled from its disassembly with PLACEHOLDER class/field names;
// only OFFSETS + code bytes are load-bearing. Engine callees are external/no-body.
#include <Ints.h>
#include <Gruntz/Effect6b.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_mgrSettings->m_logicPump @+0x78; m_tables[])
#include <Globals.h>

// ---------------------------------------------------------------------------
// 0x6b2e0: an animation effect apply - cache the owner's m_1b4 into this->m_c, run
// the owner's embedded anim sub-object (+0x1a0) advance, and (when the flag arg is
// set) re-target its draw-delta.
// @early-stop
// 76%: every instruction (lea anim, m_1b4 read, m_c store, arg push, both calls) is
// byte-faithful; the residual is pure register coloring + a 2-instr scheduling flip
// in this 0x39-byte leaf - retail keeps m_1b4 in edx and hoists the `a` load into
// eax before the m_c store; cl colors m_1b4 in eax and stores m_c first. Not
// source-steerable (every operand/declaration reorder reproduced the same coloring).
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// Advance @0x15c2d0 is CDDrawBlitParam::Setup_15c2d0, SetAnim @0x15c360 is
// CAniAdvanceCursor::Advance_15c360; TU-local decls, cast at each call.
class CDDrawBlitParamSrc;
class CDDrawBlitParam {
public:
    void Setup_15c2d0(CDDrawBlitParamSrc* src);
};
class CAniAdvanceCursor {
public:
    i32 Advance_15c360(i32 clock);
};
struct CAnimSink2 {};
struct CAnimOwner6b {
    char _00[0x1b4];
    i32 m_1b4; // +0x1b4
};

RVA(0x0006b2e0, 0x39)
void CEffect6b::Apply(i32 a, i32 b) {
    CAnimSink2* anim = (CAnimSink2*)((char*)m_4 + 0x1a0);
    m_c = m_4->m_1b4;
    ((CDDrawBlitParam*)anim)->Setup_15c2d0((CDDrawBlitParamSrc*)a);
    if (b != 0) {
        ((CAniAdvanceCursor*)anim)->Advance_15c360((i32)g_6bf3bc);
    }
}

// ---------------------------------------------------------------------------
// 0x75a40: a 2D grid lookup - bounds-check (x, y) against the width/height, then
// return the first dword of the (28-byte-stride) cell at rows[y][x]; out of bounds
// returns 1.
SIZE_UNKNOWN(CGridCell);
struct CGridCell {
    i32 m_0;
    char _pad[0x1c - 4];
};
struct CGridLookup {
    char _00[8];
    CGridCell** m_8;          // +0x08  rows
    i32 m_c;                  // +0x0c  width
    i32 m_10;                 // +0x10  height
    i32 Lookup(i32 x, i32 y); // 0x75a40
};

RVA(0x00075a40, 0x34)
i32 CGridLookup::Lookup(i32 x, i32 y) {
    if ((u32)x < (u32)m_c && (u32)y < (u32)m_10) {
        return m_8[y][x].m_0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x95140: a state-machine step - poke the input sub-object, gate on the worker
// being busy or acquirable, then run the start sequence + report. Returns 1 on the
// full path, 0 on either early-out.
struct CMenuHolder95 {
    char _00[4];
    CDDrawSubMgrPages* m_4; // +0x04
};

struct CState95 {
    char _00[4];
    CGruntzMgr* m_4; // +0x04
    char _08[0xc - 8];
    CMenuHolder95* m_c;                                    // +0x0c
    i32 Start(void* p, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x1e60
    void Report(i32 a, i32 b, i32 c, i32 d);               // 0x1843
    i32 Step(i32 arg);                                     // 0x95140
};

RVA(0x00095140, 0x6e)
i32 CState95::Step(i32 arg) {
    m_4->RestoreVideoMode(0);
    if (m_c->m_4->Method_158d20() == 0 && m_c->m_4->Method_158cb0(0, 0x30000) == 0) {
        return 0;
    }
    if (Start(&g_6111b0, 0, 0, 0, 0, 1) == 0) {
        return 0;
    }
    Report(0x50, 0x3e8, 0, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xb4350: a strike/flash effect tick - while the latch m_118 is set, pick the
// frame index (5, or 0 once a global threshold is reached) unless the strike timer
// has elapsed (which clears the latch), then seed the bound sprite's anim state
// (m_4c frame / m_50 = 7 / m_58 = 1). Always runs the trailing helper, returns 0.
DATA(0x00245588)
extern "C" u32 g_645588;   // tick
extern i32 g_strikeThresh; // 0x645598
DATA(0x0024556c)
extern CGameRegistry* g_mgrSettings; // 0x64556c
extern "C" void Helper2914();        // 0x2914 (ILT thunk)

struct CStrikeSprite {
    char _00[0x4c];
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    char _54[0x58 - 0x54];
    i32 m_58; // +0x58
};
struct CStrikeEffect {
    char _00[0x10];
    CStrikeSprite* m_10; // +0x10
    char _14[0x118 - 0x14];
    i32 m_118; // +0x118 latch
    char _11c[0x120 - 0x11c];
    i64 m_120;  // +0x120 timestamp
    i64 m_128;  // +0x128 duration
    i32 Tick(); // 0xb4350
};

// @early-stop
// 98.94%: every opcode/offset/branch is byte-identical. The lone residual is a
// load-order coin-flip in the sprite-write tail - retail reads g_mgrSettings->m_78
// (edx) before m_10 (reusing eax for the sprite ptr); cl loads m_10 first (into ecx)
// and pins the sprite there. A pure allocator choice on the [this+0x10] load; no
// source reorder flips it.
RVA(0x000b4350, 0x7e)
i32 CStrikeEffect::Tick() {
    if (m_118 != 0) {
        i32 idx = 5;
        if ((i64)(u32)g_645588 - m_120 < m_128) {
            if ((u32)g_strikeThresh >= 0x64) {
                idx = 0;
            }
        } else {
            m_118 = 0;
        }
        i32 frame = (i32)g_mgrSettings->m_logicPump->m_tables[idx];
        CStrikeSprite* spr = m_10;
        spr->m_58 = 1;
        spr->m_4c = frame;
        spr->m_50 = 7;
    }
    Helper2914();
    return 0;
}
SIZE_UNKNOWN(CAnimOwner6b);
SIZE_UNKNOWN(CAnimSink2);
SIZE_UNKNOWN(CGridLookup);
SIZE_UNKNOWN(CMenuHolder95);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CState95);
SIZE_UNKNOWN(CStrikeEffect);
SIZE_UNKNOWN(CStrikeSprite);
SIZE_UNKNOWN(CWorkerObj95);
