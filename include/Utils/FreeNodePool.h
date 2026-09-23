#ifndef GRUNTZ_UTILS_FREENODEPOOL_H
#define GRUNTZ_UTILS_FREENODEPOOL_H

#include <Ints.h>

#include <stddef.h>

template<class T> class FreeNodePool {
public:
    struct Node {
        Node* m_next;
        T m_value;
    };

    FreeNodePool() : m_block(NULL), m_freeHead(NULL), m_count(0), m_linkOffset(0) {}

    ~FreeNodePool() {
        if (m_block != NULL) {
            delete[] m_block;
        }
        m_block = NULL;
        m_freeHead = NULL;
        m_count = 0;
        m_linkOffset = 0;
    }

    bool Init(i32 count, i32 linkOffset) {
        m_block = new Node[count];
        if (m_block == NULL) {
            return false;
        }

        m_count = count;
        Node* node = m_block;
        u32 i = 0;
        do {
            node->m_next = node + 1;
            node = node->m_next;
            ++i;
        } while (i < static_cast<u32>(m_count) - 1);
        node->m_next = NULL;
        m_freeHead = m_block;
        m_linkOffset = linkOffset;
        return true;
    }

    inline void Push(void* p);

    inline T* Pop();

    Node* NodeOf(void* payload) {

        // Language-forced container-of adjustment; a union spelling changes codegen.
        return reinterpret_cast<Node*>(static_cast<char*>(payload) - m_linkOffset);
    }

    Node* m_block;
    Node* m_freeHead;
    i32 m_count;
    i32 m_linkOffset;
};

template<class T> inline T* FreeNodePool<T>::Pop() {
    Node* node = m_freeHead;
    T* result = NULL;
    if (node->m_next != NULL) {
        result = &node->m_value;
        m_freeHead = node->m_next;
    }
    return result;
}

template<class T> inline void FreeNodePool<T>::Push(void* p) {
    Node* node = NodeOf(p);
    node->m_next = m_freeHead;
    m_freeHead = node;
}

#endif // GRUNTZ_UTILS_FREENODEPOOL_H
