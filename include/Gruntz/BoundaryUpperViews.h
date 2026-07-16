// BoundaryUpperViews.h - shared referent/owner views for the upper-half
#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine).
#ifndef GRUNTZ_BOUNDARYUPPERVIEWS_H
#define GRUNTZ_BOUNDARYUPPERVIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// Embedded base-subobject vptr restamp (member dtor of the grand-base): the 7-byte
// `mov [this],&g_wapObjectDtorVtbl; ret` leaf. Three distinct leaf classes share it.
struct SW_161460 : CObject {};
SIZE_UNKNOWN(SW_161460);
struct SW_161560 : CObject {};
SIZE_UNKNOWN(SW_161560);
struct SW_163a10 : CObject {};
SIZE_UNKNOWN(SW_163a10);

// 0x13dee0 - `m_1c = v; if(v > 0) m_28 = 1000 / v;` frame-timing setter. Set (0x13dee0)'s
// body lives in src/Rez/RezMgr.cpp; the sibling TrySet (0x13df00) is still in
// BoundaryUpper.cpp, so this view stays here for it (both methods declared).
struct B_13dee0 {
    char _0[0x1c];
    i32 m_1c; // 0x1c
    char _20[0x28 - 0x20];
    i32 m_28;          // 0x28
    void Set(i32 v);   // 0x13dee0 (now in RezMgr.cpp; called out-of-line by TrySet)
    i32 TrySet(i32 v); // 0x13df00 (still here)
};
SIZE_UNKNOWN(B_13dee0);

// 0x16be60 - ButeMgr helper append.
struct C16be60 {
    i32 Ready();                     // 0x16bd10
    void Emit(const void* s, i32 a); // 0x16c2d0
    void Flush();                    // 0x16bd90
    C16be60* Append(i32 arg);
};
SIZE_UNKNOWN(C16be60);

// 0x151d20 - notify a hooked callback (stash/replace m_7c->m_1c).
struct Cb151d20 {
    char _0[0x10];
    void(__cdecl* fn)(void*); // +0x10
    char _14[0x1c - 0x14];
    void* m_1c; // 0x1c
};
SIZE_UNKNOWN(Cb151d20);
struct B_151d20 {
    char _0[0x7c];
    Cb151d20* m_7c; // 0x7c
    i32 Notify(void* arg);
};
SIZE_UNKNOWN(B_151d20);

// --- vtable catalog ---

#endif // GRUNTZ_BOUNDARYUPPERVIEWS_H
