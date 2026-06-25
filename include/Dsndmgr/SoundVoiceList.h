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

#include <Ints.h>

// The Rez heap free (0x1b9b82 _RezFree, __cdecl); reloc-masked rel32.
extern "C" void RezFree(void* p);

// The abstract-base ("pure") vftable (0x5ef6c8) a reaped element's vptr is
// restamped to before RezFree - a transitional reloc-masked DIR32 store.
extern void* const g_PureVtbl[]; // 0x5ef6c8

// A chain link as the list sees it: the biased `element+4` pointer.
struct DSoundLink {
    DSoundLink* m_next; // +0x00
    DSoundLink* m_prev; // +0x04
};

// A reaped element (RemoveMatching's view): the link occupies +0x04/+0x08 (so the
// link pointer IS element+4), a tag @+0xc and a key @+0x10. The vptr at +0x00 is
// restamped to the pure base on free.
struct DSoundElem {
    void* m_vtbl;      // +0x00
    DSoundLink m_link; // +0x04  { next@+0x04, prev@+0x08 }
    u32 m_tag; // +0x0c  (RemoveMatching skips elements whose tag != arg, unless arg==0xffff)
    u32 m_key; // +0x10
};

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

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
