#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <rva.h>

#include <Mfc.h>

struct CKeyedNode {
    CKeyedNode() {
        m_key.Empty();
        m_commandDelay = 0;
        m_resendInterval = 0;
    }

    CString m_key;
    i32 m_commandDelay;
    i32 m_resendInterval;
    CString GetName();
    ~CKeyedNode();
};

class CKeyedList {
public:
    CKeyedList(i32 nBlockSize) : m_list(nBlockSize) {
        m_mode = 0;
    }

    RVA(0x000c52b0, 0x49)
    ~CKeyedList() {
        Clear();
    }

    CKeyedNode* AddNode(const char* key, i32 commandDelay, i32 resendInterval);

    void Clear();

    CPtrList m_list;
    i32 m_mode;
};

#endif // GRUNTZ_NET_KEYEDLIST_H
