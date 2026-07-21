#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>

class CObjNode {
public:
    void* m_base;     // +0x00  element base (vptr for a polymorphic element, else data)
    CObjNode* m_next; // +0x04
    CObjNode* m_prev; // +0x08
};
SIZE_UNKNOWN(CObjNode); // a view of the (variably-sized) list elements

struct CRezListNode : public CObjNode {};
SIZE_UNKNOWN(CRezListNode);

#include <Bute/ObjListBase.h>
struct CObjList : public CObjListBase {
    // V0 (slot 0) stays pure here - CObjList is only ever a base in the Rez model.
    // NO DECLARED DESTRUCTOR (binary fact): the implicit dtor produces the same
    // inlined chain (a compiler-generated dtor stamps no vptr - the CBattlezDlg
    // rule), and cl then emits no ??_7CObjList anywhere, matching retail (whose
    // dtor chains restamp only the CObjListBase table). A user `~CObjList() {}`
    // would instead force a phantom vtable, so it is deliberately absent.
    CObjNode* m_head;            // +0x04
    CObjNode* m_tail;            // +0x08
    void Remove(CObjNode* node); // 0x1852e0
};
SIZE_UNKNOWN(CObjList); // {vptr,head,tail}=0xc header; full engine size unproven

VTBL(CRezList, 0x001ef7c8);
struct CRezList : public CObjList {
    CRezList() {
        m_head = 0;
        m_tail = 0;
    }
    virtual void V0() OVERRIDE; // [0] 0x13c4d0 (empty body; RezFile.cpp)
    // NON-virtual inline dtor: embedding dtors (~CRezDir) inline the chain; the
    // EH-funclet-referenced standalone COMDAT copy is retail 0x13ca30 (bound by
    // RezFile.cpp's @rva-symbol).
    ~CRezList() {}
    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
    // Positional inserts: splice `node` after/before `pos` (null pos -> AddHead /
    // AddTail respectively). Each branch re-reads pos->m_next/m_prev after the
    // aliasing store, and MSVC duplicates the common link tail into both arms.
    void InsertAfter(CRezListNode* pos, CRezListNode* node);  // 0x185240
    void InsertBefore(CRezListNode* pos, CRezListNode* node); // 0x185290
};
SIZE(CRezList, 0xc); // {vptr,head,tail}

#endif // REZ_REZLIST_H
