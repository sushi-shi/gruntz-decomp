#ifndef GLOBALS_H
#define GLOBALS_H

template<class T> inline T Min(T a, T b) {
    return a < b ? a : b;
}

template<class T> inline T Max(T a, T b) {
    return a > b ? a : b;
}

template<class T> inline T Clamp(T val, T min, T max) {
    return Min(max, Max(val, min));
}

#endif
