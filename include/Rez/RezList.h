#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <rva.h>

#include <Bute/ObjListBase.h>
#include <Ints.h>

#include <stddef.h>

class CRezItmBase;

union DwordBytes {
    u32 m_value;
    u8 m_bytes[4];
};

struct CObjList : public CObjListBase {

    CObjList() : m_head(NULL), m_tail(NULL) {}

    CRezItmBase* m_head;
    CRezItmBase* m_tail;
    void AddHead(CRezItmBase* node);
    void Remove(CRezItmBase* node);
};

struct CRezList : public CObjList {
    CRezList() {}
    virtual void UnusedListHook() OVERRIDE;

    RVA(0x0013cd10, 0x7)
    ~CRezList() {}
    void AddTail(CRezItmBase* node);

    void InsertAfter(CRezItmBase* pos, CRezItmBase* node);
    void InsertBefore(CRezItmBase* pos, CRezItmBase* node);
};

#endif // REZ_REZLIST_H
