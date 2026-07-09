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

// ---------------------------------------------------------------------------
// RemoveMatching (__thiscall, 2 stack args). Walk the chain; unlink +
// free every element whose key (@+0x10) equals `key` and whose tag (@+0xc) equals
// `tag` (0xffff is a wildcard). The free is `delete (PureSoundElem*)e`: the base-
// subobject teardown resets the element vptr to the pure base (??_7PureSoundElem =
// 0x5ef6c8) and PureSoundElem::operator delete RezFree's it. The tag-mismatch arm
// does not advance (it re-tests the current element) - retail's structure; the
// elements that reach here never trip it, but the source must reproduce the
// codegen, so spell it as a no-advance `continue`.
// @early-stop
// select-zero-mask-dest-register wall (docs/patterns/select-zero-mask-dest-register.md):
// byte-exact except the `e ? node : 0` mask (neg/sbb/and) lands in edx (ours) vs eax
// (retail) - a free-list pick the four obvious source spellings don't move. 99.3%,
// logic complete; deferred to the final sweep.
RVA(0x00136f60, 0x74)
void DSoundList::RemoveMatching(void* key, u32 tag) {
    DSoundElem* e = elemOf<DSoundElem>(m_head);
    while (e) {
        DSoundLink* node = &e->m_link;
        DSoundLink* n = e->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        if (tag != 0xffff && e->m_tag != tag) {
            continue;
        }
        if (e->m_key == key) {
            Unlink(e ? node : 0);
            if (e) {
                PureSoundElem* pure = e; // up-cast: teardown resets to the pure base
                delete pure;
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
// InsertAfter (0x139140, __thiscall, 2 stack args). Splice `node` in right after
// `after` (fixing the successor's prev, or the tail if `after` was last). The
// null-`after` arm calls InsertHead but shares the tail codegen (retail shape - no
// early return; callers only pass a live `after`).
RVA(0x00139140, 0x41)
void DSoundList::InsertAfter(DSoundLink* after, DSoundLink* node) {
    if (after == 0) {
        InsertHead(node);
    }
    if (after->m_next) {
        after->m_next->m_prev = node;
    } else {
        m_tail = node;
    }
    node->m_prev = after;
    node->m_next = after->m_next;
    after->m_next = node;
}

// ---------------------------------------------------------------------------
// InsertBefore (0x139190, __thiscall, 2 stack args). Splice `node` in right before
// `before` (fixing the predecessor's next, or the head if `before` was first). The
// null-`before` arm calls InsertTail but shares the tail codegen (retail shape).
RVA(0x00139190, 0x44)
void DSoundList::InsertBefore(DSoundLink* before, DSoundLink* node) {
    if (before == 0) {
        InsertTail(node);
    }
    if (before->m_prev) {
        before->m_prev->m_next = node;
    } else {
        m_head = node;
    }
    node->m_next = before;
    node->m_prev = before->m_prev;
    before->m_prev = node;
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
// Unlink (__thiscall, 1 stack arg). Splice `node` out: patch its prev's
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
