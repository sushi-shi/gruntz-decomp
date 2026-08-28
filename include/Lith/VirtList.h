#ifndef LITH_VIRTLIST_H
#define LITH_VIRTLIST_H

#include <stddef.h>

class CVirtBaseList;

class CVirtBaseListItem {
public:
    CVirtBaseListItem* Next() {
        return m_pNext;
    }

    CVirtBaseListItem* Prev() {
        return m_pPrev;
    }

    virtual void VirtualFoo() = 0;

protected:
    friend class CVirtBaseList;

    CVirtBaseListItem* m_pNext;
    CVirtBaseListItem* m_pPrev;
};

class CVirtBaseList {
public:
    CVirtBaseList() : m_pFirst(NULL), m_pLast(NULL) {}
    ~CVirtBaseList() {}

    void Insert(CVirtBaseListItem* item) {
        InsertFirst(item);
    }

    void InsertFirst(CVirtBaseListItem* item);
    void InsertLast(CVirtBaseListItem* item);
    void InsertAfter(CVirtBaseListItem* beforeItem, CVirtBaseListItem* newItem);
    void InsertBefore(CVirtBaseListItem* afterItem, CVirtBaseListItem* newItem);
    void Delete(CVirtBaseListItem* item);

    CVirtBaseListItem* GetFirst() {
        return m_pFirst;
    }

    CVirtBaseListItem* GetLast() {
        return m_pLast;
    }

    virtual void VirtualFoo() = 0;

private:
    CVirtBaseListItem* m_pFirst;
    CVirtBaseListItem* m_pLast;
};

#endif // LITH_VIRTLIST_H
