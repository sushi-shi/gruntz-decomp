// SBI_WarlordHead.h - Gruntz CSBI_WarlordHead (C:\Proj\Gruntz).
// RTTI .?AVCSBI_WarlordHead@@; the most-derived leaf of the SBI image chain
//   CSBI_WarlordHead : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5ead24 (RTTI meta 0x5f4f40). The 5-level /GX-framed scalar destructor
// (0x104a00) lives in SBI_WarlordHeadEh.cpp.
//
// This leaf adds, over CSBI_ImageSet:
//   slot 5  (0xeb880)  the per-frame Render override (countdown-driven two-frame draw)
//   slot 11 (0xeb6b0)  the SetupImage override (forwards to the ImageSet base + latches state)
// plus two non-virtual helpers (0xeb740 ShowFrames, 0xeb830 SetState).
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact (the
// mangled ?<method>@CSBI_WarlordHead@@... names are layout-independent). The class
// is modeled with the SBI family's manual-vtable-stamp device (no real `virtual`),
// so each frameless method matches without forcing a divergent compiler vtable;
// sibling/engine callees are ILT/vtable-reloc-masked.
#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <Ints.h>

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// A frame record (an element of the config record's m_14 frame table): a draw
// offset pair at m_18/m_1c and (for ShowFrames) a sprite/anim handle at m_30 whose
// m_1c is latched. Same shape the rest of the SBI image chain uses.
struct CWhFrame {
    char m_pad0[0x18];
    i32 m_18; // +0x18  x draw offset
    i32 m_1c; // +0x1c  y draw offset
    char m_pad20[0x30 - 0x20];
    i32 m_30; // +0x30  sprite/anim handle (ShowFrames latches its m_1c)
};

// The resolved config record (CSBI_Image::m_34 / the ImageSet lookup result): a
// frame-index range gate at m_64/m_68 and a frame table at m_14 (an array of
// CWhFrame*). Same CImageCfgRecord shape used by SBI_Image.cpp.
struct CWhConfig {
    char m_pad0[0x14];
    CWhFrame** m_14; // +0x14  frame table (array of frame-record pointers)
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame-index range hi gate (idx > m_64 => reject)
    i32 m_68; // +0x68  frame-index range lo gate (idx < m_68 => reject)
};

// The per-frame draw handle (m_30 latched record): RenderFrame (0x153790,
// __thiscall) blits the frame at a screen position through the surface context.
struct CWhRenderTarget {
    void RenderFrame(i32 surfaceCtx, i32 x, i32 y, i32 z); // 0x153790
};

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 dword is the
// surface context passed into RenderFrame.
struct CWhDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context
};
struct CWhGameMgr {
    char m_pad0[0x4];
    CWhDrawable* m_4; // +0x04  active drawable
};
struct CWhGameReg {
    char m_pad0[0x30];
    CWhGameMgr* m_30; // +0x30  active game manager
};

// The base ImageSet SetupImage (CSBI_ImageSet vtable slot 11, 0xe72f0): same
// 11-arg shape (id, host, a3, a4, then the four rect ints, key, a10, a11). The
// rect block is forwarded as a by-value 4-int aggregate so MSVC stages it in a
// temp on the caller stack exactly as retail does.
struct CWhRect {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
};

// The serialization stream (Serialize arg1): a real polymorphic object with
// ReadBytes at vtable slot 0x2c and WriteBytes at slot 0x30, both __thiscall
// (buf, len). Same CImageSetStream shape the rest of the SBI image chain uses;
// every call through it lowers to `mov edx,[s]; call [edx+0x2c|0x30]`.
struct CImageSetStream {
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vfa();
    virtual void ReadBytes(void* buf, i32 len);  // slot 0x2c
    virtual void WriteBytes(void* buf, i32 len); // slot 0x30
};

// The immediate base (CSBI_ImageSet, 0xe74f0 vtable slot 1). Declared only so the
// base-serialize call emits the real ?Serialize@CSBI_ImageSet@@... external; the
// reloc pairs against SBI_ImageSet.cpp's definition at 0xe74f0.
class CSBI_ImageSet {
public:
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4); // 0xe74f0
};

// ---------------------------------------------------------------------------
// CSBI_WarlordHead - the warlord-head status-bar item.
class CSBI_WarlordHead {
public:
    // vtable slot 1 (0xe7cd0): serialize the head's six persistent ints
    // (m_3c..m_50), then chain to the CSBI_ImageSet base serialize (0xe74f0).
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4);
    // vtable slot 11 (0xeb6b0): forward all 11 args to the ImageSet base setup; on
    // success latch the initial state (SetState(0)) and report 1.
    i32 SetupImage(
        i32 a1,
        i32 host,
        i32 a3,
        i32 a4,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 key,
        i32 a10,
        i32 a11
    );
    // The base ImageSet SetupImage (0xe72f0, vtable slot 11): the rect block is one
    // by-value aggregate so the temp-struct stage matches.
    i32 BaseSetupImage(i32 a1, i32 host, i32 a3, i32 a4, CWhRect rect, i32 key, i32 a10, i32 a11);

    // 0xeb740: drive the show/hide of the two anchor frames (frame table slots 1/2).
    i32 ShowFrames(i32 show, i32 arg2);
    // 0xeb830: latch the raw direction (m_3c) + the derived state (m_38 = 1 or 2).
    i32 SetState(i32 dir);
    // vtable slot 5 (0xeb880): the per-frame render of the head's two frames.
    i32 Render(i32 z);

    // The show/hide notifier for a frame's sprite handle (0x14dd90, __stdcall,
    // ret 8). Modeled as a free function so `push 0; push arg; call` falls out with
    // no this.

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    char m_pad0[0x14];
    i32 m_14; // +0x14  base rect x (draw origin)
    i32 m_18; // +0x18  base rect y (draw origin)
    char m_pad1c[0x28 - 0x1c];
    i32 m_28; // +0x28  frame countdown (Render decrements; <=0 => idle)
    char m_pad2c[0x30 - 0x2c];
    CWhFrame* m_30;  // +0x30  latched current frame record
    CWhConfig* m_34; // +0x34  resolved config record (frame table host)
    i32 m_38;        // +0x38  state/index (SetState writes 1 or 2; Render indexes by it)
    i32 m_3c;        // +0x3c  direction (SetState writes the raw dir; Render compares vs 1)
    i32 m_40;        // +0x40  persistent serialized ints (Serialize save/load block)
    i32 m_44;        // +0x44
    i32 m_48;        // +0x48
    i32 m_4c;        // +0x4c
    i32 m_50;        // +0x50
};

// The frame sprite show/hide notifier (0x14dd90, __stdcall, ret 8).
void WhShowItem(i32 handle, i32 flag);

#endif // SBI_WARLORDHEAD_H
