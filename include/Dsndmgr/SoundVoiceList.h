// SoundVoiceList.h - the WAP32 sound engine's intrusive doubly-linked-list
// primitive (C:\Proj\Dsndmgr region, RVAs 0x136f60 / 0x1390e0..0x1391e0). ONE
// physical copy of each helper; every Dsndmgr collection (the device's voice and
// buffer lists, DirectSoundMgr's clone list, SoundStream's instance list) embeds
// this {head,tail} pair as a value sub-object and calls these helpers __thiscall.
// The TUs that USE them each declare a local struct wrapper (so the call falls out
// reloc-masked); this is the home that carries the real bodies.
//
// The chain uses the engine's "POSITION +4 bias": a live element is reached
// through `element+4`, so each link/anchor is the biased pointer (the element's
// +4 word doubles as the forward link). A node, as the list sees it, is the
// 2-word link { next@+0, prev@+4 }. The list head is { head@+0, tail@+4 }.
//
// RemoveMatching (0x136f60) additionally frees each reaped element through the
// sound-sample abstract ("pure") vtable 0x5ef6c8 + RezFree; its element view adds
// a tag @+0xc (wildcard 0xffff) and a key @+0x10.
#ifndef SRC_DSNDMGR_SOUNDVOICELIST_H
#define SRC_DSNDMGR_SOUNDVOICELIST_H

#include <rva.h>

// The Rez heap free (0x1b9b82 _RezFree, __cdecl); reloc-masked rel32.
extern "C" void RezFree(void* p);

// A chain link as the list sees it: the biased `element+4` pointer.
struct DSoundLink {
    DSoundLink* m_next; // +0x00
    DSoundLink* m_prev; // +0x04
};
SIZE_UNKNOWN(DSoundLink); // 2-word intrusive chain link

// The abstract "pure" sound-element base (retail vtable 0x5ef6c8: 2 slots, both
// __purecall). ALL-VTABLES phase: modeled REAL-POLYMORPHIC (2 pure virtuals) so the
// reaped elements are real derived types; the class is never instantiated, so cl
// emits no ??_7PureSoundElem here.
struct PureSoundElem {
    virtual void Slot0() = 0; // +0x00  __purecall
    virtual void Slot1() = 0; // +0x04  __purecall
};
SIZE_UNKNOWN(PureSoundElem); // abstract element base (vptr only)

// The address of PureSoundElem's vtable (0x5ef6c8). The list reapers reset a reaped
// element's vptr to this base right before RezFree (the inlined trivial base-subobject
// dtor) - a `*(void**)e = &vtable` store MSVC's dtor codegen dead-eliminates as a dead
// store, so it stays an explicit reloc-masked DIR32 reference to the real class's table.
extern void* const PureSoundElemVtable[]; // 0x5ef6c8 = ??_7PureSoundElem@@6B@

// A reaped element (RemoveMatching's view): a PureSoundElem-derived object whose link
// occupies +0x04/+0x08 (the link pointer IS element+4), a tag @+0xc and a key @+0x10.
struct DSoundElem : PureSoundElem {
    // vptr @ +0x00 (inherited from PureSoundElem)
    DSoundLink m_link; // +0x04  { next@+0x04, prev@+0x08 }
    u32 m_tag; // +0x0c  (RemoveMatching skips elements whose tag != arg, unless arg==0xffff)
    u32 m_key; // +0x10
};
SIZE_UNKNOWN(DSoundElem); // reaped-element view (real element is larger)

// The engine's {head,tail} intrusive doubly-linked-list head.
struct DSoundList {
    DSoundLink* m_head; // +0x00
    DSoundLink* m_tail; // +0x04

    void InsertHead(DSoundLink* node); // 0x1390e0
    void InsertTail(DSoundLink* node); // 0x139110
    void Unlink(DSoundLink* node);     // 0x1391e0
    void RemoveMatching(
        u32 key, // 0x136f60
        u32 tag
    );
};
SIZE_UNKNOWN(DSoundList); // {head,tail} intrusive list head

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
