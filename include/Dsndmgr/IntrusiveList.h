#ifndef DSNDMGR_INTRUSIVELIST_H
#define DSNDMGR_INTRUSIVELIST_H

#include <rva.h>

struct IntrusiveLink {
    IntrusiveLink* m_next;
    IntrusiveLink* m_prev;
};

// Language-forced container-of adjustment at the intrusive-list boundary.
template<class T> inline T* ElementFromLink(IntrusiveLink* link) {
    return link ? reinterpret_cast<T*>((reinterpret_cast<char*>(link) - 4)) : 0;
}

struct IntrusiveList {
    IntrusiveLink* m_head;
    IntrusiveLink* m_tail;

    IntrusiveList() : m_head(0), m_tail(0) {}

    ~IntrusiveList() {}

    void InsertHead(IntrusiveLink* node);
    void InsertTail(IntrusiveLink* node);
    void InsertAfter(IntrusiveLink* after, IntrusiveLink* node);
    void InsertBefore(IntrusiveLink* before, IntrusiveLink* node);
    void Unlink(IntrusiveLink* node);
};

#endif // DSNDMGR_INTRUSIVELIST_H
