// SoundVoiceList.cpp - the WAP32 sound engine's intrusive doubly-linked-list
// primitive (the five 0x1390e0-0x1391e0 list helpers; a SEPARATE retail obj past
// the 0x13848b end of the DSndMgSR.cpp interval - see docs/exe-map/
// interval-dossiers.md). DSoundList::RemoveMatching (0x136f60) falls INSIDE the
// DSNDMGR.CPP obj span and now lives in src/Dsndmgr/DirectSoundMgr.cpp; the
// declaration stays on the shared DSoundList (SoundVoiceList.h). See
// include/Dsndmgr/SoundVoiceList.h for the layout + the call-graph evidence.
//
// The biased +/-4 element<->link conversions are the ternary short-circuit form
// (docs/patterns/biased-pointer-advance-ternary.md).
#include <Dsndmgr/SoundVoiceList.h>
#include <rva.h>

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
