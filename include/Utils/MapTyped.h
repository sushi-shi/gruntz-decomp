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
//
// This is MFC's OWN spelling, not ours: afxtempl.h's CTypedPtrMap is literally
//   BOOL Lookup(BASE_ARG_KEY key, VALUE& v) const
//       { return BASE_CLASS::Lookup(key, *(void**)&v); }
//   void GetNextAssoc(POSITION& p, KEY& k, VALUE& v) const
//       { BASE_CLASS::GetNextAssoc(p, (BASE_KEY&)k, *(void**)&v); }
// so adopting CTypedPtrMap would RELOCATE these casts into vendor MFC, not remove
// them. The only cast-free rewrite - a `void* tmp; Lookup(key, tmp); out = (T*)tmp;`
// - is strictly LESS faithful: retail passes the destination's own address to
// Lookup (`lea eax,[esi+N]`), where the temp form inserts a stack slot plus a copy
// retail does not have. Keep the pun; it is the boundary, byte-for-byte.
template<class T> inline BOOL MapLookup(CMapStringToPtr& map, LPCTSTR key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template<class T> inline BOOL MapLookup(CMapPtrToPtr& map, void* key, T*& out) {
    return map.Lookup(key, reinterpret_cast<void*&>(out));
}
template<class K, class T>
inline void MapGetNext(CMapStringToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
}
template<class K, class T>
inline void MapGetNext(CMapPtrToPtr& map, POSITION& pos, K& key, T*& out) {
    map.GetNextAssoc(pos, key, reinterpret_cast<void*&>(out));
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
// slot - the id->key pun is forced by CMapPtrToPtr's signature, so it lives here.
inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, void*& out) {
    return map.Lookup(reinterpret_cast<void*>(id), out);
}
inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, CObject*& out) {
    return map.Lookup(reinterpret_cast<void*>(id), reinterpret_cast<void*&>(out));
}
// Same seam for the game-object maps, whose out-slot is a concrete class pointer.
template<class T> inline BOOL MapLookupById(CMapPtrToPtr& map, i32 id, T*& out) {
    return map.Lookup(reinterpret_cast<void*>(id), reinterpret_cast<void*&>(out));
}

#endif // GRUNTZ_UTILS_MAPTYPED_H
