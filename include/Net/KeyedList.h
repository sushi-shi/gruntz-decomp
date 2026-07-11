// KeyedList.h - the ONE real keyed-list container (C:\Proj\NetMgr / Gruntz): an MFC
// CObList of 12-byte {CString key; int; int} payload nodes plus a latched mode int at
// +0x1c. Formerly split across two divergent views - the flat CKeyedList struct
// (KeyedList.cpp) and the fake `CLatencyList : CObList` view (LatencyList.h) - now
// unified here so CLatencyList : CKeyedList and every AddNode call binds to the one
// real 0x37a70 body. Field/method names are placeholders (campaign doctrine); only the
// offsets + emitted code bytes are load-bearing.
#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <Mfc.h> // real MFC CObList (base) + CString (the node key)
#include <rva.h>

// The payload node a key-add allocates and hangs off each CObList node's `data`
// pointer (heap, 0xc bytes): a CString key + two payload dwords. (Named CKeyedNode,
// not CNode, so it does not collide with the inherited CObList::CNode link node.)
struct CKeyedNode {
    CString m_key; // +0x00
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08
    ~CKeyedNode();
};
SIZE_UNKNOWN(CKeyedNode);

// The keyed-list container: an MFC CObList of CNode rows + the latched mode int at
// +0x1c. Allocated with block size 0xa; no own vtable (uses CObList's 0x5eb054).
SIZE(CKeyedList, 0x20);
class CKeyedList : public CObList {
public:
    // Chain the MFC CObList(nBlockSize) ctor (0x1b4867), then zero the mode.
    CKeyedList(i32 nBlockSize) : CObList(nBlockSize) {
        m_mode = 0;
    }

    // 0x37a70: new a {key,a2,a3} CKeyedNode, assign it, AddTail it; returns the node.
    CKeyedNode* AddNode(const char* key, i32 a2, i32 a3);

    // 0x379a0: free every node's owned payload (dtor + operator delete), RemoveAll the
    // backing CObList, zero the mode.
    void Clear();

    i32 m_mode; // +0x1c  latched mode
};

#endif // GRUNTZ_NET_KEYEDLIST_H
