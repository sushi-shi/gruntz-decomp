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
#include <Gruntz/SoundCueMgr.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

// The +0x24 config host is the shared canonical CSbiConfigHost (SbiConfig.h, pulled
// in the .cpp); only a pointer is needed here, so forward-declare it.
struct CSbiConfigHost;

// ---------------------------------------------------------------------------
// Engine-referent views the reconstructed CSBI_MenuItem methods drive (modeled
// minimally; the methods/fields touched are the only load-bearing facts - every
// call through them is reloc-masked). Moved here from the per-TU inline defs so
// they carry a single shared definition.

// A resolved cue record: a player at +0x10 plus a draw-clock gate (+0x14 last,
// +0x18 interval).
struct CSoundCueMgr; // defined below
struct CMiCue {
    char m_pad0[0x10];
    CSoundCueMgr* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;           // +0x14  last draw-clock
    i32 m_18;           // +0x18  interval
};
SIZE_UNKNOWN(CMiCue);

// The cue lookup map embedded at the music host's +0x10 (CMapStringToOb::Lookup,
// 0x1b8438, ret 8) - the cue-facet map, distinct from the image registry's m_10map.
struct CMiCueMap {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at each call
SIZE_UNKNOWN(CMiCueMap);

// The music host reached as g_gameReg->m_world->m_28 viewed as its cue facet: a
// non-null +0x30 gate suppresses the cue play; the cue map is the sub-object at
// host+0x10 (documented sub-object offset). This is the SAME +0x28 sound object as
// CResMgr::m_28 (CSoundRegistry, the install facet); its cue map's Lookup (0x1b8438)
// differs from the install facet's (0x1b8008), so the cue view is reached by a
// documented multi-view cast on m_28 - the cross-TU merge with SBI_RectOnly's
// identical CSbiMusicHost is deferred (see report).
struct CMiMusicHost {
    char m_pad0[0x30];
    i32 m_30; // +0x30  reentrancy gate flag (opaque; only null-tested => skip)
};
SIZE_UNKNOWN(CMiMusicHost);

// The owning rect-only host at m_2c: SetState drives its tab state through three
// sibling thunks (all ILT-reloc-masked). m_10c is the active-tab latch.
struct CMiTabHost {
    // TabBegin @0x100b00 IS CSBI_RectOnly::ClearTabGroup; cast at the call.
    // TabRefresh @0x102250 IS CStatusBarMgr::LoadTabSprites; cast at the call.
    // TabCommit @0x100cb0 IS CSBI_RectOnly::Deactivate; cast at the call.
    char m_pad0[0x10c];
    i32 m_10c; // +0x10c  active-tab latch
};
SIZE_UNKNOWN(CMiTabHost);

// A polymorphic view of `this` used only for the self-virtual slot-0x28 dispatch:
// 10 leading slots + Refresh at index 10 (byte 0x28). Declared (never defined) so
// no ??_7 is emitted here; `((CMiSelf*)this)->Refresh()` lowers to the exact
// mov eax,[this]; mov ecx,this; call [eax+0x28] __thiscall dispatch.
class CMiSelf {
public:
    virtual void Destroy();     // slot 0  scalar-deleting dtor
    virtual void Serialize();   // slot 1
    virtual void Setup();       // slot 2
    virtual void ClearFrame();  // slot 3
    virtual void Poll();        // slot 4
    virtual void Tick();        // slot 5
    virtual void HitHandlerA(); // slot 6
    virtual void HitHandlerB(); // slot 7
    virtual void HitHandlerC(); // slot 8
    virtual void HitHandlerD(); // slot 9
    virtual void Refresh();     // +0x28 (slot 10)
};
SIZE_UNKNOWN(CMiSelf);

// The frame-name reverse-lookup (0x155630) on the config registry is
// CImageRegistry::ReadField (mgr->m_10, <Gruntz/ResMgr.h>); the former CMiNameReg
// view is gone (wave 3).

// The archive object passed to Serialize is the shared WAP32 CSerialArchive (Read @
// vtable +0x2c / Write @ +0x30), now the one modeled class in <Gruntz/SerialArchive.h>
// - the former local `CMiArchive` view is folded away.

// ---------------------------------------------------------------------------
// CSBI_MenuItem - a single status-bar menu entry. The small layout (fields up to
// +0x38) holds: a config-host pointer at +0x24, an item-counter at +0x28, an
// owning rect-only host at +0x2c, the resolved frame at +0x30, the menu state
// tag at +0x34, and a resolved cue/config record at +0x38.
//
// Real RTTI base is CSBI_Image (see top comment); kept FLAT (frameless method-view)
// because the matched methods read nearly the whole base region (m_4..m_30) under
// menu-item-specific names, so deriving CSBI_Image would erase those recovered names.
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
    virtual void
    VSlot0(); // +0x00  manual-stamp vtable pointer (slot 0x28 = Refresh)  // real polymorphic vptr @+0x00 (was m_vptr)
    i32 m_4;  // +0x04  active/valid flag
    i32 m_8;  // +0x08  subtype tag (=2)
    i32 m_c;  // +0x0c  command/tab id
    i32 m_10; // +0x10  arg0
    i32 m_14; // +0x14  rect x0 / arg block start
    i32 m_18; // +0x18  rect y0
    i32 m_1c; // +0x1c  rect x1
    i32 m_20; // +0x20  rect y1
    CSbiConfigHost* m_24; // +0x24  config host
    i32 m_28;             // +0x28  counter / subtype
    CMiTabHost* m_2c;     // +0x2c  owning rect-only host (CSBI_Image-like, tab-state view)
    i32 m_30;             // +0x30  resolved frame handle (CImage* stored as DWORD)
    i32 m_34;             // +0x34  menu state tag
    // +0x38 is a PROVEN-heterogeneous slot: ResolveFrame stores a CSbiConfigRecord*,
    // Serialize (case 7) stores the CSprite the image registry yields. Same physical
    // shape, distinct modeled classes reached on different code paths -> kept void*.
    void* m_38; // +0x38  resolved cue/config record (config-record | sprite)
};
SIZE_UNKNOWN(CSBI_MenuItem);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CMiSelf, 0x001eab4c);

#endif // SBI_MENUITEM_H
