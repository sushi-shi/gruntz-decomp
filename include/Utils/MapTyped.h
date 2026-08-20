#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H

#include <Mfc.h>

#include <AddrWord.h>
#include <Ints.h>

template<class T> union MapOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

template<class T> inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, T*& out) {
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    return map.Lookup(key, *dst.m_asVoid);
}
template<class T> inline BOOL MapLookup(CMapPtrToPtr& map, void* key, T*& out) {
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    return map.Lookup(key, *dst.m_asVoid);
}
template<class K, class T>
inline void MapGetNext(CMapStringToPtr& map, POSITION& pos, K& key, T*& out) {
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    map.GetNextAssoc(pos, key, *dst.m_asVoid);
}
template<class K, class T>
inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, T*& out) {
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    map.GetNextAssoc(pos, key, *dst.m_asVoid);
}

inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, CObject*& out) {
    AddrWord<char> k;
    k.m_word = id;
    MapOutRef<CObject> dst;
    dst.m_asTyped = &out;
    return map.Lookup(k.m_addr, *dst.m_asVoid);
}

template<class T> inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, T*& out) {
    AddrWord<char> k;
    k.m_word = id;
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    return map.Lookup(k.m_addr, *dst.m_asVoid);
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
