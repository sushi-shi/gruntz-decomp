// KeyedList.h - the ONE real keyed-list container (C:\Proj\NetMgr / Gruntz): an MFC
// CPtrList of 12-byte {CString key; int; int} payload nodes plus a latched mode int at
// +0x1c. Formerly split across two divergent views - the flat CKeyedList struct
// (KeyedList.cpp) and the fake `CLatencyList : CPtrList` view (LatencyList.h) - now
// unified here so CLatencyList : CKeyedList and every AddNode call binds to the one
// real 0x37a70 body. Field/method names are placeholders (campaign doctrine); only the
// offsets + emitted code bytes are load-bearing.
#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <Mfc.h> // real MFC CPtrList (base) + CString (the node key)
#include <rva.h>

// The payload node a key-add allocates and hangs off each CPtrList node's `data`
// pointer (heap, 0xc bytes): a CString key + two payload dwords. (Named CKeyedNode,
// not CNode, so it does not collide with the inherited CPtrList::CNode link node.)
struct CKeyedNode {
    CString m_key; // +0x00
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08
    ~CKeyedNode();
};
SIZE_UNKNOWN(CKeyedNode);

// The keyed-list container: an MFC CPtrList of CNode rows + the latched mode int at
// +0x1c. Allocated with block size 0xa. NOT CPtrList-DERIVED - the list is a MEMBER
// (binary proof, 2026-07-19): retail's user-written dtor 0xc5280 (EH frame + Clear +
// ~CPtrList) carries NO vptr re-stamp, and the ctor inlined into BuildSlotList
// (0xc1e60) stamps only CPtrList's own 0x5eb054 - a CPtrList-DERIVED class would be
// polymorphic and MSVC5 stamps ??_7CKeyedList in BOTH (the CBattlezDlg-measured rule;
// the derived model sat at 91.6%/88.2% on exactly those extra stamps and emitted a
// ??_7CKeyedList retail does not have). A user dtor of a NON-polymorphic class
// destroying a polymorphic member reproduces every byte.
SIZE(CKeyedList, 0x20);
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

#endif // GRUNTZ_NET_KEYEDLIST_H
