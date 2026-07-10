// BoundaryTailViews.h - shared referent/owner views for the hard CString/CSymParser/
// CImage tail of the engine_boundary vein reconstructed in BoundaryTail.cpp (DinMgr2 /
// Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral) and gives the /GX EH-frame sibling (BoundaryTailEh.cpp) /
// a final sweep one definition to reuse.
#ifndef GRUNTZ_BOUNDARYTAILVIEWS_H
#define GRUNTZ_BOUNDARYTAILVIEWS_H

#include <rva.h>
#include <Mfc.h> // real MFC CString (copy-ctor 0x1b9ba3 / dtor 0x1b9cde, reloc-masked)

// 0x176d20 CImage scanline fill + 0x176da0 wrapper: folded onto the real CRezImage
// (src/Image/ImageRectFill.cpp, <Image/Image.h>) - the delinker-guessed CImg176d20 /
// FillRect176d20 placeholders removed.

// 0x788d0 - sound-emitter screen-position update.
struct Emitter788d0 {
    char _0[8];
    u8 m_8; // flags (bit 0)
    char _9[0x10 - 9];
    float m_10, m_14, m_18, m_1c; // 0x10,0x14,0x18,0x1c
};
SIZE_UNKNOWN(Emitter788d0);
struct ElemSrc788d0 {
    char _0[0x5c];
    i32 m_5c, m_60; // 0x5c,0x60
};
SIZE_UNKNOWN(ElemSrc788d0);
struct Elem788d0 {
    char _0[0x10];
    ElemSrc788d0* m_10; // 0x10
};
SIZE_UNKNOWN(Elem788d0);
struct Holder788d0_24 {
    char _0[0x5c];
    Emitter788d0* m_5c; // 0x5c
};
SIZE_UNKNOWN(Holder788d0_24);
struct Holder788d0 {
    char _0[0x24];
    Holder788d0_24* m_24; // 0x24
};
SIZE_UNKNOWN(Holder788d0);
struct CSnd788d0 {
    char _0[0x1c];
    Elem788d0* m_1c[1]; // 0x1c array
    char _pad[0x22c - 0x1c - 4];
    Holder788d0* m_22c; // 0x22c
    char _230[4];       // 0x230
    i32 m_234;          // 0x234
    i32 m_238;          // 0x238
    i32 PositionUpdate();
};
SIZE_UNKNOWN(CSnd788d0);

// 0x85500 - return a CString member BY VALUE (offset 0xec). (0x38120 was folded onto
// the real CLatencyItem::GetName in src/Gruntz/SlotComboFill.cpp - view removed.)
struct Obj85500 {
    char _0[0xec];
    CString m_ec; // 0xec
    CString GetName();
};
SIZE_UNKNOWN(Obj85500);

// 0x148250 - CDDPalette::Flush: RE-HOMED to the directdrawmgr unit
// (src/DDrawMgr/DirectDrawMgr.cpp, real CDDPalette in DirectDrawMgr.h).

// 0x23d90 - snap a draw rectangle to the 0x20 grid and dispatch a blit.
struct R23d90 {
    char _0[0x40];
    i32 m_40, m_44; // 0x40,0x44
};
SIZE_UNKNOWN(R23d90);
struct P23d90 {
    char _0[0x10];
    i32 m_10, m_14; // 0x10,0x14 origin
    char _18[0x5c - 0x18];
    R23d90* m_5c; // 0x5c bounds
};
SIZE_UNKNOWN(P23d90);
struct Mid23d90 {
    char _0[0x24];
    P23d90* m_24; // 0x24
};
SIZE_UNKNOWN(Mid23d90);
struct Outer23d90 {
    char _0[0x30];
    Mid23d90* m_30; // 0x30
};
SIZE_UNKNOWN(Outer23d90);
struct CObj23d90 {
    char _0[0x38];
    Outer23d90* m_38; // 0x38
    void Blit(i32 a1, i32 a2, i32 x, i32 y, i32 a5);
};
SIZE_UNKNOWN(CObj23d90);

// 0xbdd0 - look a key up in arg1's embedded map (at +0x10) then dispatch.
struct Entry_bdd0 {
    char _0[0x10];
    void* m_10; // 0x10
};
SIZE_UNKNOWN(Entry_bdd0);
struct Map_bdd0 {
    // Lookup @0x1b8438 IS CMapStringToOb::Lookup; cast at the call.
};
SIZE_UNKNOWN(Map_bdd0);
struct Arg1_bdd0 {
    char _0[0x10];
    Map_bdd0 m_10; // 0x10 (Map_bdd0)
};
SIZE_UNKNOWN(Arg1_bdd0);
struct CObj_bdd0 {
    void* DispatchEntry(void* a, i32 b, i32 c, i32 d, i32 e); // 0x3026
    void* Dispatch(Arg1_bdd0* a1, const char* key, i32 a3, i32 a4, i32 a5, i32 a6);
};
SIZE_UNKNOWN(CObj_bdd0);

// 0x118330 - populate an output record from three successive iterator reads.
struct Node118330 {
    char _0[0xc];
    i32 m_c, m_10, m_14; // 0xc,0x10,0x14
};
SIZE_UNKNOWN(Node118330);
struct Iter118330 {
    void* pos;
    Node118330* GetNext(int x); // 0x1b30f0 (__thiscall)
};
SIZE_UNKNOWN(Iter118330);
struct Out118330 {
    char _0[0xc];
    i32 m_c, m_10, m_14; // 0xc,0x10,0x14
};
SIZE_UNKNOWN(Out118330);

#endif // GRUNTZ_BOUNDARYTAILVIEWS_H
