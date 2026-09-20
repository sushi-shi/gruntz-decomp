#ifndef GRUNTZ_ACTREGISTRY_H
#define GRUNTZ_ACTREGISTRY_H

#include <AddrWord.h>
#include <ZTools/PTree.h>

extern zSymTab<i32> g_buteTree;

static inline i32 ActFindId(const char* key) {
    AddrWord<i32> v;
    v.m_addr = g_buteTree.lookup(key);
    return v.m_word;
}
static inline void ActInsertId(const char* key, i32 id) {
    AddrWord<i32> v;
    v.m_word = id;
    g_buteTree.add(key, v.m_addr);
}

#endif // GRUNTZ_ACTREGISTRY_H
