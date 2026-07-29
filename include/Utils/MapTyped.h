#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H
#include <Mfc.h>
#include <Ints.h>
#include <AddrWord.h> // the id-word / void*-key pair

// Typed views over MFC's void*-out-param map API.
//
// CMapStringToPtr/CMapPtrToPtr hand their value back through `void*&`, and C++
// cannot bind a `T*&` to a `void*&` - so a reinterpret is language-forced here.
// These wrappers keep that ONE forced pun at the boundary instead of repeating
// it at every lookup site, and they make the stored element type explicit.
//
// Adopting afxtempl.h's CTypedPtrMap would only RELOCATE the pun into vendor MFC -
// its own body is `BASE_CLASS::Lookup(key, *(void**)&v)` - and the obvious cast-free
// rewrite (`void* tmp; Lookup(key, tmp); out = (T*)tmp;`) is strictly LESS faithful:
// retail passes the DESTINATION'S OWN ADDRESS to Lookup (`lea eax,[esi+N]`), where
// the temp form inserts a stack slot plus a copy retail does not have.
//
// So the destination slot's ADDRESS is what carries two readings - MFC writes a
// void* there, the caller reads a T* - and naming both keeps retail's shape with no
// pun at all. MapOutRef does exactly that, once, for every wrapper below.
template<class T> union MapOutRef {
    void** m_asVoid; // what MFC's `void*&` out-parameter binds to
    T** m_asTyped;   // the caller's own destination variable
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

// The already-void* out-param needs no pun at all.
inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, void*& out) {
    return map.Lookup(key, out);
}
inline BOOL MapLookup(CMapPtrToPtr& map, void* key, void*& out) {
    return map.Lookup(key, out);
}
template<class K> inline void MapGetNext(CMapStringToPtr& map, POSITION& pos, K& key, void*& out) {
    map.GetNextAssoc(pos, key, out);
}
template<class K> inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, void*& out) {
    map.GetNextAssoc(pos, key, out);
}

// The object maps are keyed by the object's serial ID, which MFC stores in a void*
// slot - the same word read both ways, which AddrWord names (<AddrWord.h>).
inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, void*& out) {
    AddrWord k;
    k.m_word = id;
    return map.Lookup(k.m_addr, out);
}
inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, CObject*& out) {
    AddrWord k;
    k.m_word = id;
    MapOutRef<CObject> dst;
    dst.m_asTyped = &out;
    return map.Lookup(k.m_addr, *dst.m_asVoid);
}
// Same seam for the game-object maps, whose out-slot is a concrete class pointer.
template<class T> inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, T*& out) {
    AddrWord k;
    k.m_word = id;
    MapOutRef<T> dst;
    dst.m_asTyped = &out;
    return map.Lookup(k.m_addr, *dst.m_asVoid);
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
