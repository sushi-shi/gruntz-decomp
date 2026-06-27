// CObjectDropper.cpp - the object-dropper tile-logic object (C:\Proj\Gruntz), a
// CUserLogic leaf. The /GX leaf dtor + the 1-arg ctor (shared CUserLogic(obj)
// prologue + the per-class drop setup).
#include <Gruntz/CObjectDropper.h>

#include <string.h> // inline strcmp for the direction-name match

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190) + the bute manager
// (g_buteMgr.GetDwordDef 0x1721e0); declared extern so the calls reloc-mask.
extern CButeTree g_buteTree;
extern CButeMgr g_buteMgr;

// The CString temp the direction-name match builds. Its three operations are the
// static-linked MFC CString helpers, modeled NO-body so the calls reloc-mask:
//   MiniStr()         = 0x1b9b93 (default-construct / empty)
//   operator=(LPCSTR) = 0x1b9e74 (assign the C string)
//   ~MiniStr()        = 0x1b9cde (destruct; the /GX temp-cleanup state)
// The real C++ dtor makes MSVC emit the temp's EH cleanup state like retail.
struct MiniStr {
    char* m_buf; // +0x00  the buffer pointer (the strcmp operand)
    MiniStr();
    ~MiniStr();
    MiniStr& operator=(const char* s);
};

// The bound CGameObject viewed by the ctor (m_10 == m_38). Only the touched
// offsets are modeled.
struct CObjDropObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08 flags
    char m_pad0c[0x4c - 0x0c];
    i32 m_4c; // +0x4c sprite-ref handle
    i32 m_50; // +0x50 state
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58 active flag
    i32 m_5c; // +0x5c screen X
    i32 m_60; // +0x60 screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74 layer key
    char m_pad78[0x12c - 0x78];
    i32 m_12c; // +0x12c travel direction (1..4)
    char m_pad130[0x144 - 0x130];
    i32 m_144; // +0x144 rect base
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    char m_pad154[0x194 - 0x154];
    void* m_194; // +0x194 the sprite/name record (dir name at +0x24)
    char m_pad198[0x1b4 - 0x198];
    i32 m_1b4; // +0x1b4 cycle-geometry id
};

// The global game registry (WwdGameReg, RVA 0x24556c). m_134 == 1 selects the
// scroll mode; m_78 is the level sprite-ref/selector table.
struct CObjDropReg {
    char m_pad00[0x78];
    i32* m_78; // +0x78 selector table
    char m_pad7c[0x134 - 0x7c];
    i32 m_134; // +0x134 mode discriminator
};
DATA(0x0024556c)
extern CObjDropReg* g_gameReg;

// 1000.0 (the per-tile-time -> per-frame-speed reciprocal numerator), VA 0x5ea9f0.
DATA(0x001ea9f0)
extern const double g_objDropDiv;

// CObjectDropper::~CObjectDropper (0x124f0) - the /GX leaf dtor folds the bare
// CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the
// +0x18 link (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr
// (0x5e70b4). The leaf vptr store is dead-eliminated.
RVA(0x000124f0, 0x44)
CObjectDropper::~CObjectDropper() {}

// CObjectDropper::CObjectDropper @0xc59f0 - fold the shared CUserLogic(obj) init,
// bind the cycle geometry + "A" bute node, snap the bound object to the tile grid,
// match the dropper's direction name (LEVEL_OBJECTDROPPER_{NORTH,EAST,SOUTH,WEST})
// to seed the travel vector + direction id, then read the per-tile time
// (ObjectDropperTimePerTile) into the per-frame speed and seed the bound sprite's
// draw state.
//
// @early-stop
// inline-strcmp + register-pinning + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops, the CString temp lifecycle + EH states,
// the snap/bute/time-divide all match retail). Residual is the strcmp result-reg
// alloc (ebx pinned to 1 across the first loop), the /GX leaf-vptr re-stamp position
// + the shared dy=0 store fold. Not source-steerable (global regalloc/EH numbering).
RVA(0x000c59f0, 0x3e3)
CObjectDropper::CObjectDropper(CGameObject* obj) : CUserLogic(obj) {
    m_88 = 0.0;
    m_90 = 0.0;
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("LEVEL_OBJECTDROPPER", 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 0x2000002;

    CObjDropObj* o = (CObjDropObj*)m_10;
    i32 snapX = (o->m_5c & ~0x1f) + 0x10;
    i32 snapY = (o->m_60 & ~0x1f) + 0x10;
    o->m_5c = snapX;
    m_60 = (double)snapX;
    o->m_60 = snapY;
    m_68 = (double)snapY;
    if (o->m_74 != 0xcf851) {
        o->m_74 = 0xcf851;
        o->m_08 |= 0x20000;
    }

    CObjDropObj* obj38 = (CObjDropObj*)m_38;
    if (obj38->m_194 != 0) {
        MiniStr name;
        name = (char*)obj38->m_194 + 0x24;
        const char* s = name.m_buf;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            o->m_12c = 1;
            m_70 = 0;
            m_74 = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            o->m_12c = 2;
            m_70 = 1;
            m_74 = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            o->m_12c = 3;
            m_70 = 0;
            m_74 = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            o->m_12c = 4;
            m_70 = -1;
            m_74 = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_80 = 0;
    m_78 = -1;
    m_7c = -1;
    m_58 = g_objDropDiv / (double)(i64)(u32)time;
    if (g_gameReg->m_134 == 1) {
        m_80 = 1;
    }
    i32 sel = g_gameReg->m_78[10];
    o->m_58 = 1;
    o->m_50 = 7;
    o->m_4c = sel;
    m_88 = 0.0;
    m_90 = 0.0;
    o->m_144 = 1;
    o->m_14c = 1;
    o->m_148 = 1;
    o->m_150 = 1;
}
