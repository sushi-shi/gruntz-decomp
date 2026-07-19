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
// sound-sample abstract ("pure") base PureSoundElem + the Rez heap; its element
// view adds a tag @+0xc (wildcard 0xffff) and a key @+0x10.
#ifndef SRC_DSNDMGR_SOUNDVOICELIST_H
#define SRC_DSNDMGR_SOUNDVOICELIST_H

#include <rva.h>

// The Rez heap free (0x1b9b82 RezFree, __cdecl); reloc-masked rel32. Reached
// through PureSoundElem::operator delete (below) so a `delete (PureSoundElem*)e`
// lowers to the retail "reset vptr to the pure base, then RezFree" teardown.
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// A chain link as the list sees it: the biased `element+4` pointer.
struct DSoundLink {
    DSoundLink* m_next; // +0x00
    DSoundLink* m_prev; // +0x04
};
SIZE(DSoundLink, 0x8); // 2-word intrusive chain link

// The engine's POSITION +4 bias, as one typed CONTAINING_RECORD accessor: a live
// element is reached through `element + 4` (its DSoundLink sits at +0x04, right after
// the vptr), so the list threads `element + 4` pointers and the element is `link - 4`.
// Every reap/tick walk recovers its typed element through this instead of open-coding
// the pointer arithmetic.
template<class T> inline T* elemOf(DSoundLink* link) {
    return link ? (T*)(reinterpret_cast<char*>(link) - 4) : 0;
}

// The abstract "pure" sound-element base (retail vtable 0x5ef6c8: 2 __purecall
// slots). Real-polymorphic: Tick/Stop are pure, so a derived voice/element
// overrides them and the class is never instantiated (cl emits ??_7PureSoundElem
// only where a `delete` inlines the base-subobject teardown). A NON-virtual dtor +
// a class operator delete (RezFree) mean `delete (PureSoundElem*)e` lowers to the
// retail inline "mov [e],??_7PureSoundElem; RezFree(e)" - the reap teardown, with
// NO manual vtable store.
struct PureSoundElem {
    virtual i32 Tick(i32 now) = 0; // +0x00  slot 0  __purecall (per-frame update)
    virtual i32 Stop() = 0;        // +0x04  slot 1  __purecall
    void operator delete(void* p); // Rez-heap free (RezFree, 0x1b9b82)
    // NON-virtual dtor (the 2-slot vtable holds no dtor). Inline+empty so every
    // `delete (PureSoundElem*)e` keeps the retail inlined teardown (vptr reset to
    // ??_7PureSoundElem + RezFree). Retail also carries ONE standalone out-of-line
    // COMDAT copy at 0x137330 (7 B: `mov [ecx],??_7PureSoundElem; ret`), emitted in
    // the DSndMgSR.cpp obj because its EH unwind funclet (0x1e0950) takes the
    // dtor's address; SoundStream.cpp's @rva-symbol names that retail copy (was
    // the fake placeholder class CAbstract137330).
    ~PureSoundElem() {}
};
inline void PureSoundElem::operator delete(void* p) {
    RezFree(p);
}
SIZE(PureSoundElem, 0x4);        // one vptr (abstract element base)
VTBL(PureSoundElem, 0x001ef6c8); // 2 __purecall slots (Tick/Stop); the reap-teardown
                                 // delete-sites (DirectSoundMgr/SoundDevice/SoundVoiceList)
                                 // emit ??_7PureSoundElem, delinked at 0x1ef6c8.

// A reaped element (RemoveMatching's view): a PureSoundElem-derived object whose
// link occupies +0x04/+0x08 (the link pointer IS element+4), a tag @+0xc and a key
// @+0x10. RemoveMatching frees it through `delete (PureSoundElem*)e` - the generic
// reset-to-pure-base + RezFree teardown.
struct DSoundElem : public PureSoundElem {
    // vptr @ +0x00 (inherited from PureSoundElem)
    DSoundLink m_link; // +0x04  { next@+0x04, prev@+0x08 }
    u32 m_tag;         // +0x0c  (skipped unless tag == arg, or arg == 0xffff)
    void* m_key;       // +0x10  match key (the owning buffer pointer)
};
SIZE(DSoundElem, 0x14); // reaped-element view (real element may add trailing fields)

// The engine's {head,tail} intrusive doubly-linked-list head.
struct DSoundList {
    DSoundLink* m_head; // +0x00
    DSoundLink* m_tail; // +0x04

    void InsertHead(DSoundLink* node);                       // 0x1390e0
    void InsertTail(DSoundLink* node);                       // 0x139110
    void InsertAfter(DSoundLink* after, DSoundLink* node);   // 0x139140
    void InsertBefore(DSoundLink* before, DSoundLink* node); // 0x139190
    void Unlink(DSoundLink* node);                           // 0x1391e0
    void RemoveMatching(void* key, u32 tag);                 // 0x136f60
};
SIZE(DSoundList, 0x8); // {head,tail} intrusive list head

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
