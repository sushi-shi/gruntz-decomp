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
// The class sits on the real polymorphic canonical chain (SBI_Image.h) since the
// *Eh.cpp collapse - the compiler emits its ??_7 from the sema-proven slot decls.
// Offsets are the load-bearing fact; field names are placeholders recovered from
// the matched use-sites.
#ifndef SBI_MENUITEM_H
#define SBI_MENUITEM_H

#include <Ints.h>
#include <Gruntz/SoundCueMgr.h>
#include <rva.h>

#include <Gruntz/SBI_Image.h> // canonical chain base CSBI_Image : CSBI_RectOnly : CStatusBarItem
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

// The +0x24 config host is the shared canonical CSpriteFactoryHolder (SbiConfig.h, pulled
// in the .cpp); only a pointer is needed here, so forward-declare it.
struct CSpriteFactoryHolder;

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
// (The ex-`CMapStringToOb` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToOb::Lookup @0x1b8438 - the member is the real map.)

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
// CSBI_MenuItem - a single status-bar menu entry, on the real RTTI chain
// CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem. The inherited base
// region carries: m_4 active flag, m_8 subtype tag (=2), m_c command/tab id, m_10
// arg0, m_rect14 the rect block (x0/y0/x1/y1; .m_4/.m_8 double as the frame draw
// origin), m_24 the config host (CSpriteFactoryHolder*, i32 slot - cast at the deref like
// the sibling leaves), m_28 the counter, m_2c the owning tab host (CMiTabHost view
// at the deref), m_30 the resolved frame handle (CImage* stored as DWORD, the
// CSBI_Image slot). Adds the menu state tag (+0x34) and the resolved cue/config
// record (+0x38).
//
// Formerly kept FLAT to preserve the menu-item field names; folded onto the
// canonical chain by the *Eh.cpp collapse (this TU also owns the /GX chain dtor
// 0x1007d0, which requires the real polymorphic base chain).
// ---------------------------------------------------------------------------
class CSBI_MenuItem : public CSBI_Image {
public:
    // Real vtable shape (sema class: vtbl@0x1eab4c, 12 slots; overrides 0/1/3/4/5/11).
    // The out-of-line ~ (0x1007d0, calls ClearFrame2) lives in SBI_MenuItem.cpp via
    // the CHAIN-DTOR device (see StatusBarItem.h).
    virtual ~CSBI_MenuItem() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;  // slot 1 (the Serialize below)
    virtual void SbiSlot3() OVERRIDE;  // slot 3 (the ClearFrame2 below)
    virtual void SbiSlot4() OVERRIDE;  // slot 4 (the DecCounter below)
    virtual void SbiSlot5() OVERRIDE;  // slot 5
    // slot 11 (0xe80e0), the CSBI_Image::SetupImage override. This USED to be split in two:
    // a body-less `virtual` here to pin the slot, plus the real body as a separate
    // non-virtual `InitItem`. Its arg model looked different (it called arg9 `obj` and
    // arg10 `key`) but is the SAME: arg1 owner, arg2 host, arg3 cmd, arg4 -> m_10,
    // args5..8 the rect, and its `ResolveFrame(obj, key)` is really
    // ResolveFrame(<asset key>, <frame index>) - i.e. arg9 IS the key and arg10 the frame,
    // exactly as the base and CSBI_ImageSet have it. One function, one slot.
    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CSpriteFactoryHolder* host,
        i32 cmd,
        i32 a4,
        SbRect rc,
        const char* key,
        i32 frame,
        i32 unused
    ) OVERRIDE; // slot 11  0xe80e0

    // ----- reconstructed methods (RVA-ascending) -----
    // (0xe6d90 ClearFrame + 0xe6e40 SerializeChain are the real CSBI_Image slot-3/
    // slot-1 bodies - re-attributed to SBI_Image.h/.cpp, dossier #16.)
    void ClearFrame2(); // 0xe81a0 (out-of-line)
    // (InitItem was the real 0xe80e0 body under a second name - it IS the slot-11
    // SetupImage override declared above.)
    i32 ResolveFrame(i32 key, i32 a);                // 0xe81e0
    i32 DecCounter();                                // 0xe82a0  decrement-and-blit
    i32 SetState(i32 state, i32 a);                  // 0xe8310
    i32 ProbeState(i32 state);                       // 0xe8480
    i32 Blit();                                      // 0xe84f0  conditional blit
    i32 Serialize(void* ar, i32 kind, i32 a, i32 b); // 0xe8520  top serialize
    void SetSubtype();                               // 0x1005b0 (out-of-line)
    // (0x10bfc0 SerializeFields is the real CStatusBarItem slot-1 base leg -
    // decl on the base in StatusBarItem.h; body stays in SBI_MenuItem.cpp.)

    // ----- own fields (after CSBI_Image @0x34) -----
    i32 m_34; // +0x34  menu state tag
    // +0x38 is a PROVEN-heterogeneous slot: ResolveFrame stores a CSbiConfigRecord*,
    // Serialize (case 7) stores the CSprite the image registry yields. Same physical
    // shape, distinct modeled classes reached on different code paths -> kept void*.
    void* m_38; // +0x38  resolved cue/config record (config-record | sprite)
};
SIZE_UNKNOWN(CSBI_MenuItem);
VTBL(CSBI_MenuItem, 0x001eab4c); // vtable_names -> code (RTTI game class)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SBI_MENUITEM_H
