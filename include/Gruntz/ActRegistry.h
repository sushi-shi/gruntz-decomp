#ifndef GRUNTZ_ACTREGISTRY_H
#define GRUNTZ_ACTREGISTRY_H

#include <Ints.h>
#include <ZTools/PTree.h>

extern zSymTab<i32> g_buteTree;

static inline i32 ActFindId(const char* key) {

    return reinterpret_cast<i32>(g_buteTree.lookup(key));
}
static inline void ActInsertId(const char* key, i32 id) {

    g_buteTree.add(key, reinterpret_cast<i32*>(id));
}

#endif
