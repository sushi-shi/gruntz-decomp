#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <Mfc.h>
#include <rva.h>

struct CKeyedNode {
    CString m_key;
    i32 m_4;
    i32 m_8;
    ~CKeyedNode();
};
SIZE_UNKNOWN();

class CKeyedList {
public:
    CKeyedList(i32 nBlockSize) : m_list(nBlockSize) {
        m_mode = 0;
    }

    ~CKeyedList();

    CKeyedNode* AddNode(const char* key, i32 a2, i32 a3);

    void Clear();

    CPtrList m_list;
    i32 m_mode;
};
SIZE(0x20);

#endif // GRUNTZ_NET_KEYEDLIST_H
