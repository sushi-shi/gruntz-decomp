#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>
#include <rva.h>
#include <Bute/ObjListBase.h>

// The intrusive element. CRezItmBase IS the list's node type - it is not a separate
// `CObjNode` overlay. PROVEN (2026-07-27), replacing the old "language-forced pun"
// note that kept 16 reinterpret_casts alive:
//   - AddHead (0x1851e0) and Remove (0x1852e0) touch ONLY node+0x04 / node+0x08, which
//     are exactly CRezItmBase::m_next / m_prev; nothing ever reads node+0x00, so the
//     ex-CObjNode's `m_base` field had zero binary backing;
//   - every retail caller of both (ParseBuffer / LoadEntry / ~CSymParser / Clear /
//     CRezFile's ctor+dtor / OpenFile / CloseFile - `sema xref`) enrols a
//     CRezItmBase-derived element (CRezDir, CRezItm, CRezFile);
//   - CObjList's own m_head/m_tail are read back as CRezItmBase* at every use.
// The one apparent counter-example (CMapMgr::Search handing its result to a list) is
// not one: retail's call there is ?AddHead@CPtrList@@ (0x1b4967), a real MFC CPtrList.
class CRezItmBase;

// A dword and its four bytes are the same storage - the BUTE tag/fourcc paths read
// it both ways, so the two arms are a real union rather than a pun.
union DwordBytes {
    u32 m_v;
    u8 m_b[4];
};

struct CObjList : public CObjListBase {
    // V0 (slot 0) stays pure here - CObjList is only ever a base in the Rez model.
    // NO DECLARED DESTRUCTOR (binary fact): the implicit dtor produces the same
    // inlined chain (a compiler-generated dtor stamps no vptr - the CBattlezDlg
    // rule), and cl then emits no ??_7CObjList anywhere, matching retail (whose
    // dtor chains restamp only the CObjListBase table). A user `~CObjList() {}`
    // would instead force a phantom vtable, so it is deliberately absent.
    CRezItmBase* m_head;             // +0x04
    CRezItmBase* m_tail;             // +0x08
    void AddHead(CRezItmBase* node); // 0x1851e0
    void Remove(CRezItmBase* node);  // 0x1852e0
};
SIZE(0xc); // {vptr (CObjListBase), head, tail} - CRezList adds no data, same 0xc

struct CRezList : public CObjList {
    CRezList() {
        m_head = 0;
        m_tail = 0;
    }
    virtual void V0() OVERRIDE; // [0] 0x13c4d0 (empty body; RezFile.cpp)
    // NON-virtual inline dtor: embedding dtors (~CRezDir) inline the chain; the
    // EH-funclet-referenced standalone COMDAT copy is retail 0x13ca30 (bound by
    // RezFile.cpp's RVA_COMPGEN).
    ~CRezList() {}
    void AddTail(CRezItmBase* node); // 0x185210
    // Positional inserts: splice `node` after/before `pos` (null pos -> AddHead /
    // AddTail respectively). Each branch re-reads pos->m_next/m_prev after the
    // aliasing store, and MSVC duplicates the common link tail into both arms.
    void InsertAfter(CRezItmBase* pos, CRezItmBase* node);  // 0x185240
    void InsertBefore(CRezItmBase* pos, CRezItmBase* node); // 0x185290
};
SIZE(0xc); // {vptr,head,tail}

#endif // REZ_REZLIST_H
