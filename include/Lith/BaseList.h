#ifndef LITH_BASELIST_H
#define LITH_BASELIST_H

#include <stddef.h>

class CBaseListItem {
public:
    CBaseListItem* Next() {
        return m_pNext;
    }

    CBaseListItem* Prev() {
        return m_pPrev;
    }

protected:
    friend class CLTBaseList;

    CBaseListItem* m_pNext;
    CBaseListItem* m_pPrev;
};

class CLTBaseList {
public:
    CLTBaseList() : m_pFirst(NULL), m_pLast(NULL) {}
    ~CLTBaseList() {}

    void Insert(CBaseListItem* item) {
        InsertFirst(item);
    }

    void InsertFirst(CBaseListItem* item);
    void InsertLast(CBaseListItem* item);
    void InsertAfter(CBaseListItem* beforeItem, CBaseListItem* newItem);
    void InsertBefore(CBaseListItem* afterItem, CBaseListItem* newItem);
    void Delete(CBaseListItem* item);

    CBaseListItem* GetFirst() {
        return m_pFirst;
    }

    CBaseListItem* GetLast() {
        return m_pLast;
    }

    void FastDeleteAll() {
        m_pFirst = NULL;
        m_pLast = NULL;
    }

protected:
    CBaseListItem* m_pFirst;
    CBaseListItem* m_pLast;
};

#endif // LITH_BASELIST_H
