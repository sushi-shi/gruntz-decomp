#include <rva.h>

#include <Lith/VirtList.h>

#include <stddef.h>

RVA(0x001851e0, 0x2a)
void CVirtBaseList::InsertFirst(CVirtBaseListItem* item) {
    item->m_pNext = m_pFirst;
    item->m_pPrev = NULL;
    if (m_pFirst != NULL) {
        m_pFirst->m_pPrev = item;
    } else {
        m_pLast = item;
    }
    m_pFirst = item;
}

RVA(0x00185210, 0x2a)
void CVirtBaseList::InsertLast(CVirtBaseListItem* item) {
    item->m_pNext = NULL;
    item->m_pPrev = m_pLast;
    if (m_pLast != NULL) {
        m_pLast->m_pNext = item;
    } else {
        m_pFirst = item;
    }
    m_pLast = item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185240, 0x48)
void CVirtBaseList::InsertAfter(CVirtBaseListItem* beforeItem, CVirtBaseListItem* newItem) {
    if (beforeItem == NULL) {
        InsertFirst(newItem);
    }
    if (beforeItem->m_pNext != NULL) {
        beforeItem->m_pNext->m_pPrev = newItem;
    } else {
        m_pLast = newItem;
    }
    newItem->m_pPrev = beforeItem;
    newItem->m_pNext = beforeItem->m_pNext;
    beforeItem->m_pNext = newItem;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185290, 0x48)
void CVirtBaseList::InsertBefore(CVirtBaseListItem* afterItem, CVirtBaseListItem* newItem) {
    if (afterItem == NULL) {
        InsertLast(newItem);
    }
    if (afterItem->m_pPrev != NULL) {
        afterItem->m_pPrev->m_pNext = newItem;
    } else {
        m_pFirst = newItem;
    }
    newItem->m_pNext = afterItem;
    newItem->m_pPrev = afterItem->m_pPrev;
    afterItem->m_pPrev = newItem;
}

RVA(0x001852e0, 0x35)
void CVirtBaseList::Delete(CVirtBaseListItem* item) {
    if (item->m_pPrev != NULL) {
        item->m_pPrev->m_pNext = item->m_pNext;
    } else {
        m_pFirst = item->m_pNext;
    }
    if (item->m_pNext != NULL) {
        item->m_pNext->m_pPrev = item->m_pPrev;
    } else {
        m_pLast = item->m_pPrev;
    }
}
