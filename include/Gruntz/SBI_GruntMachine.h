// SBI_GruntMachine.h - Gruntz CSBI_GruntMachine (C:\Proj\Gruntz).
// RTTI .?AVCSBI_GruntMachine@@; a sibling leaf of the SBI family
//   CSBI_GruntMachine : CStatusBarItem  (RTTI hierarchy: {CSBI_GruntMachine,
//   CStatusBarItem}). Vtable @0x5eadbc (RTTI meta 0x5f4fa0). The /GX-framed scalar
// destructor (0x104ce0) lives in SBI_GruntMachineEh.cpp.
//
// The "grunt machine" status item: a short two-frame anim driven by a countdown.
// SetFrames primes the two frame indices and arms the countdown (m_28=2); each
// Render tick decrements the countdown, resolves the two indexed frame records
// through the config record's gated frame table, and blits up to three frames (the
// standalone handle m_44, plus the two resolved records) at the base origin offset
// by each record's own draw delta. Modeled with the SBI family's manual-vtable-stamp
// device (no real `virtual`); sibling/engine callees are ILT/vtable-reloc-masked.
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact, the
// mangled (?<method>@CSBI_GruntMachine@@...) name is layout-independent.
#ifndef SBI_GRUNTMACHINE_H
#define SBI_GRUNTMACHINE_H

#include <Ints.h>
#include <rva.h>

#include <Image/CImage.h> // the canonical frame-record class (CImage::RenderFrame @0x153790)

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call/datum through them is reloc-masked).

// The frame record (an element of the config record's m_14 frame table, and the
// type of the standalone m_44 handle) is the RTTI-confirmed CImage: a draw-offset
// pair at m_18/m_1c, drawn by CImage::RenderFrame (0x153790, __thiscall). Modeled
// by the shared <Image/CImage.h> definition; every call through it is reloc-masked.

// The resolved config record (CSBI_GruntMachine::m_30): a frame-index range gate at
// m_64/m_68 and a frame table at m_14 (an array of CImage*). Same CSbiConfigRecord
// shape as CSbiConfigRecord (<Gruntz/SbiConfig.h>).
struct CGmConfig {
    char m_pad0[0x14];
    CImage** m_14; // +0x14  frame table (array of frame-record pointers)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame-index range lo gate (idx < m_64 => reject)
    i32 m_68; // +0x68  frame-index range hi gate (idx > m_68 => reject)
};
SIZE_UNKNOWN(CGmConfig);

// The active drawable reached via g_gameReg->m_world->m_4: its +0x14 dword is the
// surface context passed into RenderFrame.
struct CGmDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context
};
SIZE_UNKNOWN(CGmDrawable);
struct CGmGameMgr {
    char m_pad0[0x4];
    CGmDrawable* m_4; // +0x04  active drawable
};
SIZE_UNKNOWN(CGmGameMgr);
struct CGmGameReg {
    char m_pad0[0x30];
    CGmGameMgr* m_world; // +0x30  active game manager
};
SIZE_UNKNOWN(CGmGameReg);

// ---------------------------------------------------------------------------
// CSBI_GruntMachine - the grunt-machine status-bar item. Derives directly from
// CStatusBarItem (vtable @0x5eadbc).
class CSBI_GruntMachine {
public:
    // vtable slot 3 (0xe8c70): drop the standalone frame handle + the two resolved
    // frame records (also reached by the destructor as the member teardown).
    void Reset();
    // 0xe8dc0 (__thiscall, ret 8): prime the two frame indices (each gated by != -1)
    // and arm the countdown (m_28 = 2).
    void SetFrames(i32 idxA, i32 idxB);
    // vtable slot 5 (0xe8cb0): the per-frame render of the machine's frames.
    i32 Render(i32 z);

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad0[0x14];
    i32 m_14; // +0x14  base draw origin x
    i32 m_18; // +0x18  base draw origin y
    char m_pad1c[0x28 - 0x1c];
    i32 m_28; // +0x28  frame countdown (Render decrements; <=0 => idle)
    char m_pad2c[0x30 - 0x2c];
    CGmConfig* m_30; // +0x30  resolved config record (frame table host)
    CImage* m_34;    // +0x34  resolved frame for index m_38
    i32 m_38;        // +0x38  frame index A (resolved into m_34)
    CImage* m_3c;    // +0x3c  resolved frame for index m_40
    i32 m_40;        // +0x40  frame index B (resolved into m_3c)
    CImage* m_44;    // +0x44  standalone frame handle (blitted directly)
};
SIZE_UNKNOWN(CSBI_GruntMachine);

#endif // SBI_GRUNTMACHINE_H
