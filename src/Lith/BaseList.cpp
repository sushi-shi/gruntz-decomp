#include <rva.h>

#include <Lith/BaseList.h>

#include <stddef.h>

RVA(0x001390e0, 0x25)
void CLTBaseList::InsertFirst(CBaseListItem* item) {
    item->m_pNext = m_pFirst;
    item->m_pPrev = NULL;
    if (m_pFirst) {
        m_pFirst->m_pPrev = item;
    } else {
        m_pLast = item;
    }
    m_pFirst = item;
}

RVA(0x00139110, 0x27)
void CLTBaseList::InsertLast(CBaseListItem* item) {
    item->m_pNext = NULL;
    item->m_pPrev = m_pLast;
    if (m_pLast) {
        m_pLast->m_pNext = item;
    } else {
        m_pFirst = item;
    }
    m_pLast = item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139140, 0x41)
void CLTBaseList::InsertAfter(CBaseListItem* beforeItem, CBaseListItem* newItem) {
    if (beforeItem == NULL) {
        InsertFirst(newItem);
    }
    if (beforeItem->m_pNext) {
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
RVA(0x00139190, 0x44)
void CLTBaseList::InsertBefore(CBaseListItem* afterItem, CBaseListItem* newItem) {
    if (afterItem == NULL) {
        InsertLast(newItem);
    }
    if (afterItem->m_pPrev) {
        afterItem->m_pPrev->m_pNext = newItem;
    } else {
        m_pFirst = newItem;
    }
    newItem->m_pNext = afterItem;
    newItem->m_pPrev = afterItem->m_pPrev;
    afterItem->m_pPrev = newItem;
}

RVA(0x001391e0, 0x30)
void CLTBaseList::Delete(CBaseListItem* item) {
    if (item->m_pPrev) {
        item->m_pPrev->m_pNext = item->m_pNext;
    } else {
        m_pFirst = item->m_pNext;
    }
    if (item->m_pNext) {
        item->m_pNext->m_pPrev = item->m_pPrev;
    } else {
        m_pLast = item->m_pPrev;
    }
}
