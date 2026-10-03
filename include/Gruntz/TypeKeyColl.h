#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <map>
#include <string>

#include <Ints.h>

#include <Gruntz/ActRegistry.h>

extern std::map<i32, std::string> g_typeColl;

extern i32 g_typeCounter;

inline const std::string& GetAnimationActName(i32 id) {
    return g_typeColl[id];
}

#endif
