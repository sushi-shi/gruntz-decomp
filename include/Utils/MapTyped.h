#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H
#include <Mfc.h>
#include <Ints.h>

// Typed views over MFC's void*-out-param map API.
//
// CMapStringToPtr/CMapPtrToPtr hand their value back through `void*&`, and C++
// cannot bind a `T*&` to a `void*&` - so a reinterpret is language-forced here.
// These wrappers keep that ONE forced pun at the boundary instead of repeating
// it at every lookup site, and they make the stored element type explicit.
template <class T>
inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template <class T>
inline BOOL MapLookup(CMapPtrToPtr& map, void* key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template <class K, class T>
inline void MapGetNext(CMapStringToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
}
template <class K, class T>
inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
