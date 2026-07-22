#ifndef SRC_DSNDMGR_SOUNDVOICELIST_H
#define SRC_DSNDMGR_SOUNDVOICELIST_H

#include <rva.h>

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

struct DSoundLink {
    DSoundLink* m_next; // +0x00
    DSoundLink* m_prev; // +0x04
};
SIZE(0x8); // 2-word intrusive chain link

template<class T> inline T* elemOf(DSoundLink* link) {
    return link ? reinterpret_cast<T*>((reinterpret_cast<char*>(link) - 4)) : 0;
}

struct PureSoundElem {
    virtual i32 Tick(i32 now) = 0; // +0x00  slot 0  __purecall (per-frame update)
    virtual i32 Stop() = 0;        // +0x04  slot 1  __purecall
    void operator delete(void* p); // Rez-heap free (RezFree, 0x1b9b82)
    // NON-virtual dtor (the 2-slot vtable holds no dtor). Inline+empty so every
    // `delete (PureSoundElem*)e` keeps the retail inlined teardown (vptr reset to
    // ??_7PureSoundElem + RezFree). Retail also carries ONE standalone out-of-line
    // COMDAT copy at 0x137330 (7 B: `mov [ecx],??_7PureSoundElem; ret`), emitted in
    // the DSndMgSR.cpp obj because its EH unwind funclet (0x1e0950) takes the
    // dtor's address; SoundStream.cpp's RVA_COMPGEN names that retail copy (was
    // the fake placeholder class CAbstract137330).
    ~PureSoundElem() {}
};
SIZE(0x4); // one vptr (abstract element base)
inline void PureSoundElem::operator delete(void* p) {
    RezFree(p);
}

struct DSoundElem : public PureSoundElem {
    // vptr @ +0x00 (inherited from PureSoundElem)
    DSoundLink m_link; // +0x04  { next@+0x04, prev@+0x08 }
    u32 m_tag;         // +0x0c  (skipped unless tag == arg, or arg == 0xffff)
    void* m_key;       // +0x10  match key (the owning buffer pointer)
};
SIZE(0x14); // reaped-element view (real element may add trailing fields)

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
SIZE(0x8); // {head,tail} intrusive list head

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
