#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <Mfc.h> // real MFC CPtrList (base) + CString (the node key)
#include <rva.h>

struct CKeyedNode {
    CString m_key; // +0x00
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08
    ~CKeyedNode();
};
SIZE_UNKNOWN();

class CKeyedList {
public:
    // Run the MFC CPtrList(nBlockSize) ctor (0x1b4867) on the member, zero the mode.
    CKeyedList(i32 nBlockSize) : m_list(nBlockSize) {
        m_mode = 0;
    }

    // 0xc5280: Clear() + the implicit member ~CPtrList (this IS the former CNetThing dtor).
    ~CKeyedList();

    // 0x37a70: new a {key,a2,a3} CKeyedNode, assign it, AddTail it; returns the node.
    CKeyedNode* AddNode(const char* key, i32 a2, i32 a3);

    // 0x379a0: free every node's owned payload (dtor + operator delete), RemoveAll the
    // backing CPtrList, zero the mode.
    void Clear();

    CPtrList m_list; // +0x00  the backing MFC list (vtable 0x5eb054 stamped by ITS ctor)
    i32 m_mode;      // +0x1c  latched mode
};
SIZE(0x20);

#endif // GRUNTZ_NET_KEYEDLIST_H
