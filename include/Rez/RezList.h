#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>
#include <Bute/ObjListBase.h>

class CRezItmBase;

union DwordBytes {
    u32 m_v;
    u8 m_b[4];
};

struct CObjList : public CObjListBase {

    CRezItmBase* m_head;
    CRezItmBase* m_tail;
    void AddHead(CRezItmBase* node);
    void Remove(CRezItmBase* node);
};
SIZE(0xc);

struct CRezList : public CObjList {
    CRezList() {
        m_head = 0;
        m_tail = 0;
    }
    virtual void V0() OVERRIDE;

    ~CRezList() {}
    void AddTail(CRezItmBase* node);

    void InsertAfter(CRezItmBase* pos, CRezItmBase* node);
    void InsertBefore(CRezItmBase* pos, CRezItmBase* node);
};
SIZE(0xc);

#endif // REZ_REZLIST_H
