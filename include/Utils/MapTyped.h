#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H

#include <Ints.h>

// Give MFC a void* object to write, then convert its value to the stored type.
template<class T> inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, T*& out) {
    void* value;
    BOOL found = map.Lookup(key, value);
    if (found) {
        out = static_cast<T*>(value);
    }
    return found;
}
template<class T> inline BOOL MapLookup(CMapPtrToPtr& map, void* key, T*& out) {
    void* value;
    BOOL found = map.Lookup(key, value);
    if (found) {
        out = static_cast<T*>(value);
    }
    return found;
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
    void* value;
    map.GetNextAssoc(pos, key, value);
    out = static_cast<T*>(value);
}
template<class K, class T>
inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, T*& out) {
    void* value;
    map.GetNextAssoc(pos, key, value);
    out = static_cast<T*>(value);
}

template<class T> inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, T*& out) {
    // API-forced: CMapPtrToPtr keys an integer id through its void* key.
    void* value;
    BOOL found = map.Lookup(reinterpret_cast<void*>(id), value);
    if (found) {
        out = static_cast<T*>(value);
    }
    return found;
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
