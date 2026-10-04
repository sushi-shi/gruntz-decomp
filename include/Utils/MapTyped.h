#ifndef GRUNTZ_UTILS_MAPTYPED_H
#define GRUNTZ_UTILS_MAPTYPED_H

#include <map>
#include <string>
#include <Ints.h>

template<class Map, class Key, class T>
inline bool MapLookup(const Map& values, const Key& key, T*& out) {
    typename Map::const_iterator found = values.find(key);
    if (found == values.end()) return false;
    out = static_cast<T*>(found->second);
    return true;
}

template<class T, class Map, class Key>
inline T* MapFind(const Map& values, const Key& key) {
    T* result = NULL;
    MapLookup(values, key, result);
    return result;
}

template<class Map, class T>
inline bool MapLookupById(const Map& values, i32 id, T*& out) {
    return MapLookup(values, id, out);
}

#endif
