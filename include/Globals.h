#ifndef GLOBALS_H
#define GLOBALS_H

#include <Win32.h>

#include <math.h>

inline BOOL PtInRect(const RECT* pRect, int x, int y) {
    if (x >= pRect->right || x < pRect->left || y >= pRect->bottom || y < pRect->top) {
        return FALSE;
    }

    return TRUE;
}
template<class TYPE> class CRange {
public:
    CRange() {}

    CRange(TYPE fMin, TYPE fMax) {
        m_fMin = fMin;
        m_fMax = fMax;
    }

    void Set(TYPE fMin, TYPE fMax) {
        m_fMin = fMin;
        m_fMax = fMax;
    }

    TYPE GetMin() {
        return m_fMin;
    }

    TYPE GetMax() {
        return m_fMax;
    }

protected:
    TYPE m_fMin;
    TYPE m_fMax;
};

template<class T> inline T MinAbs(T a, T b) {
    return static_cast<T>(fabs(a)) < static_cast<T>(fabs(b)) ? a : b;
}

template<class T> inline T MaxAbs(T a, T b) {
    return static_cast<T>(fabs(a)) > static_cast<T>(fabs(b)) ? a : b;
}

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
