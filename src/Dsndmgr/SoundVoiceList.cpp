// SoundVoiceList.cpp - the WAP32 sound engine's intrusive doubly-linked-list
// primitive (the home for the four shared list helpers used across Dsndmgr). See
// include/Dsndmgr/SoundVoiceList.h for the layout + the call-graph evidence
// (SoundDevice/SoundStream/DirectSoundMgr each declare a local wrapper struct over
// these same RVAs). Recovered from the "ClassUnknown_30" trace group; the four
// helpers are ONE physical copy each, modeled here on the {head,tail} list head.
//
// The biased +/-4 element<->link conversions are the ternary short-circuit form
// (docs/patterns/biased-pointer-advance-ternary.md).
#include <Dsndmgr/SoundVoiceList.h>
#include <rva.h>

// The abstract-base ("pure") vftable (0x5ef6c8) a reaped element's vptr is
// restamped to before RezFree - a transitional reloc-masked DIR32 store.
DATA(0x001ef6c8)
extern void* const g_PureVtbl[];

// ---------------------------------------------------------------------------
// RemoveMatching (0x136f60, __thiscall, 2 stack args). Walk the chain; unlink +
// free every element whose key (@+0x10) equals `key` and whose tag (@+0xc) equals
// `tag` (0xffff is a wildcard). The free restamps the element vptr to the pure
// base then RezFree's it. The tag-mismatch arm does not advance (it re-tests the
// current element) - retail's structure; the elements that reach here never trip
// it, but the source must reproduce the codegen, so spell it as a no-advance
// `continue`.
// @early-stop
// select-zero-mask-dest-register wall (docs/patterns/select-zero-mask-dest-register.md):
// byte-exact except the `e ? node : 0` mask (neg/sbb/and) lands in edx (ours) vs eax
// (retail) - a free-list pick the four obvious source spellings don't move. 99.3%,
// logic complete; deferred to the final sweep.
RVA(0x00136f60, 0x74)
void DSoundList::RemoveMatching(u32 key, u32 tag) {
    DSoundElem* e = m_head ? (DSoundElem*)((char*)m_head - 4) : 0;
    while (e) {
        DSoundLink* node = &e->m_link;
        DSoundLink* n = e->m_link.m_next;
        DSoundElem* next = n ? (DSoundElem*)((char*)n - 4) : (DSoundElem*)n;
        if (tag != 0xffff && e->m_tag != tag) {
            continue;
        }
        if (e->m_key == key) {
            Unlink(e ? node : 0);
            if (e) {
                e->m_vtbl = (void*)g_PureVtbl;
                RezFree(e);
            }
        }
        e = next;
    }
}

// ---------------------------------------------------------------------------
// InsertHead (0x1390e0, __thiscall, 1 stack arg). Prepend `node`: node->next =
// head; node->prev = 0; fix the old head's prev (or the tail if empty); head = node.
RVA(0x001390e0, 0x25)
void DSoundList::InsertHead(DSoundLink* node) {
    node->m_next = m_head;
    node->m_prev = 0;
    if (m_head) {
        m_head->m_prev = node;
    } else {
        m_tail = node;
    }
    m_head = node;
}

// ---------------------------------------------------------------------------
// InsertTail (0x139110, __thiscall, 1 stack arg). Append `node`: node->next = 0;
// node->prev = tail; fix the old tail's next (or the head if empty); tail = node.
RVA(0x00139110, 0x27)
void DSoundList::InsertTail(DSoundLink* node) {
    node->m_next = 0;
    node->m_prev = m_tail;
    if (m_tail) {
        m_tail->m_next = node;
    } else {
        m_head = node;
    }
    m_tail = node;
}

// ---------------------------------------------------------------------------
// Unlink (0x1391e0, __thiscall, 1 stack arg). Splice `node` out: patch its prev's
// next (or the head) and its next's prev (or the tail).
RVA(0x001391e0, 0x30)
void DSoundList::Unlink(DSoundLink* node) {
    if (node->m_prev) {
        node->m_prev->m_next = node->m_next;
    } else {
        m_head = node->m_next;
    }
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    } else {
        m_tail = node->m_prev;
    }
}
