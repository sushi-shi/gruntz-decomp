#ifndef GRUNTZ_ACTREGISTRY_H
#define GRUNTZ_ACTREGISTRY_H

#include <Ints.h>
#include <ZTools/PTree.h>

extern zSymTab<i32> g_buteTree;

static inline i32 ActFindId(const char* key) {
    // PROVEN: action IDs occupy pointer-valued slots in the 32-bit symbol table.
    return reinterpret_cast<i32>(g_buteTree.lookup(key));
}
static inline void ActInsertId(const char* key, i32 id) {
    // PROVEN: this slot carries an integer action ID, not an owned pointee.
    g_buteTree.add(key, reinterpret_cast<i32*>(id));
}

#endif // GRUNTZ_ACTREGISTRY_H
