#ifndef GRUNTZ_ACTREGISTRY_H
#define GRUNTZ_ACTREGISTRY_H

#include <string>

#include <Ints.h>
#include <ZTools/PTree.h>

extern zSymTab<i32> g_buteTree;

static inline i32 ActFindId(const std::string& key) {

    return reinterpret_cast<i32>(g_buteTree.lookup(key.c_str()));
}
static inline void ActInsertId(const std::string& key, i32 id) {

    g_buteTree.add(key.c_str(), reinterpret_cast<i32*>(id));
}

#endif
