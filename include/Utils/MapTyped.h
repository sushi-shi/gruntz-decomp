#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H

#include <Mfc.h>

#include <AddrWord.h>
#include <Ints.h>

// The MFC maps write pointer values through their native void*& output parameters.
template<class T> inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template<class T> inline BOOL MapLookup(CMapPtrToPtr& map, void* key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template<class T> inline T* MapFind(CMapStringToOb& map, LPCTSTR key) {
    CObject* found = NULL;
    map.Lookup(key, found);
    return static_cast<T*>(found);
}

template<class T> inline T* MapFind(CMapStringToPtr& map, LPCTSTR key) {
    T* found = NULL;
    MapLookup(map, key, found);
    return found;
}

template<class K, class T>
inline void MapGetNext(CMapStringToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
}
template<class K, class T>
inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
}

template<class T> inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, T*& out) {
    AddrWord<char> k;
    k.m_word = id;
    return map.Lookup(k.m_addr, reinterpret_cast<void*&>(out));
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
