#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <Mfc.h>
#include <rva.h>

struct CKeyedNode {
    CString m_key;
    i32 m_commandDelay;
    i32 m_drainReload;
    CString GetName();
    ~CKeyedNode();
};
SIZE_UNKNOWN();

class CKeyedList {
public:
    CKeyedList(i32 nBlockSize) : m_list(nBlockSize) {
        m_mode = 0;
    }

    ~CKeyedList();

    CKeyedNode* AddNode(const char* key, i32 commandDelay, i32 drainReload);

    void Clear();

    CPtrList m_list;
    i32 m_mode;
};
SIZE(0x20);

#endif // GRUNTZ_NET_KEYEDLIST_H
