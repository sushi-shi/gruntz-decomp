// SBI_MenuItem.h - Gruntz CSBI_MenuItem (C:\Proj\Gruntz), RTTI .?AVCSBI_MenuItem@@.
//
// The most-derived member of the status-bar-item family:
//   CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem
// proven by the destructor at 0x1007d0, which unwinds the subobject chain by
// re-stamping the four base vtables 0x5eab4c -> 0x5eac0c -> 0x5eab8c -> 0x5eabcc
// (CSBI_MenuItem / CSBI_Image / CSBI_RectOnly / CStatusBarItem). The sibling
// "CSBI_RectOnly.cpp" TU actually models CSBI_ImageSet (vtable 0x5eac4c); this
// menu-item class is distinct.
//
// The class is modeled with the manual-vtable-stamp device shared across the SBI
// family (no real `virtual`), so its concrete methods can be matched without
// emitting a divergent compiler vtable. Offsets are the load-bearing fact; field
// names are placeholders recovered from the matched use-sites.
#ifndef SBI_MENUITEM_H
#define SBI_MENUITEM_H

#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CSBI_MenuItem - a single status-bar menu entry. The small layout (fields up to
// +0x38) holds: a config-host pointer at +0x24, an item-counter at +0x28, an
// owning rect-only host at +0x2c, the resolved frame at +0x30, the menu state
// tag at +0x34, and a resolved cue/config record at +0x38.
// ---------------------------------------------------------------------------
class CSBI_MenuItem {
public:
    // ----- reconstructed methods (RVA-ascending) -----
    void ClearFrame();                                    // 0xe6d90  [+0x30]=0
    void ClearFrame2();                                   // 0xe81a0  [+0x30]=0 (sibling subobject)
    i32 SerializeChain(void* ar, i32 kind, i32 a, i32 b); // 0xe6e40  serialize tail-chain
    i32 InitItem(
        i32 cfg,
        i32 host,
        i32 cmd,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3,
        i32 r4,
        void* obj,
        i32 key,
        i32 unused
    );                                                     // 0xe80e0
    i32 ResolveFrame(i32 key, i32 a);                      // 0xe81e0
    i32 DecCounter();                                      // 0xe82a0  decrement-and-blit
    i32 SetState(i32 state, i32 a);                        // 0xe8310
    i32 ProbeState(i32 state);                             // 0xe8480
    i32 Blit();                                            // 0xe84f0  conditional blit
    i32 Serialize(void* ar, i32 kind, i32 a, i32 b);       // 0xe8520  top serialize
    void SetSubtype();                                     // 0x1005b0  [+0x28]=2
    i32 SerializeFields(void* ar, i32 kind, i32 a, i32 b); // 0x10bfc0  field block

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    void* m_vptr; // +0x00  manual-stamp vtable pointer (slot 0x28 = Refresh)
    i32 m_4;      // +0x04  active/valid flag
    i32 m_8;      // +0x08  subtype tag (=2)
    i32 m_c;      // +0x0c  command/tab id
    i32 m_10;     // +0x10  arg0
    i32 m_14;     // +0x14  rect x0 / arg block start
    i32 m_18;     // +0x18  rect y0
    i32 m_1c;     // +0x1c  rect x1
    i32 m_20;     // +0x20  rect y1
    void* m_24;   // +0x24  config host
    i32 m_28;     // +0x28  counter / subtype
    void* m_2c;   // +0x2c  owning rect-only host (CSBI_Image-like)
    i32 m_30;     // +0x30  resolved frame handle
    i32 m_34;     // +0x34  menu state tag
    void* m_38;   // +0x38  resolved cue/config record
};
SIZE_UNKNOWN(CSBI_MenuItem);

#endif // SBI_MENUITEM_H
