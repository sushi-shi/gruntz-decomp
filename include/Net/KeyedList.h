#ifndef GRUNTZ_NET_KEYEDLIST_H
#define GRUNTZ_NET_KEYEDLIST_H

#include <list>
struct CKeyedNode;

#include <string>

#include <Ints.h>

struct CKeyedNode {
    CKeyedNode() {
        (m_key).erase();
        m_commandDelay = 0;
        m_resendInterval = 0;
    }

    std::string m_key;
    i32 m_commandDelay;
    i32 m_resendInterval;
    std::string GetName();
    i32 GetCommandDelay() const {
        return m_commandDelay;
    }
    i32 GetResendInterval() const {
        return m_resendInterval;
    }
    ~CKeyedNode();
};

class CKeyedList {
public:
    CKeyedList() {
        m_mode = 0;
    }

    ~CKeyedList() {
        Clear();
    }

    CKeyedNode* AddNode(const std::string& key, i32 commandDelay, i32 resendInterval);

    void Clear();

    std::list<CKeyedNode*> m_list;
    i32 m_mode;
};

#endif
