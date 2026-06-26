// RezList.h - the Rez subsystem's intrusive doubly-linked list (C:\Proj\...\Rez).
//
// A header object whose head/tail live at +4/+8 and whose elements carry their own
// next/prev links at +4/+8 (the CRezFile open-file list links each file in directly;
// CRezFile::Open at 0x13cdc0 AddHead()s the file into mgr->m_18->m_10). AddHead /
// AddTail / Remove (0x1851e0/0x185210/0x1852e0) are non-virtual __thiscall (ret 4).
// Offsets + code bytes are the load-bearing facts; the class/field names are
// descriptive placeholders.
#ifndef REZ_REZLIST_H
#define REZ_REZLIST_H

#include <Ints.h>

// An element's intrusive links: next at +4, prev at +8.
struct CRezListNode {
    void* m_0;
    CRezListNode* m_4; // +0x04  next
    CRezListNode* m_8; // +0x08  prev
};

// The list header: head at +4, tail at +8.
struct CRezList {
    void* m_0;
    CRezListNode* m_4; // +0x04  head
    CRezListNode* m_8; // +0x08  tail

    void AddHead(CRezListNode* node); // 0x1851e0
    void AddTail(CRezListNode* node); // 0x185210
};

#endif // REZ_REZLIST_H
