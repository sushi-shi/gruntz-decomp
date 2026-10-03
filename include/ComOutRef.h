#ifndef GRUNTZ_COMOUTREF_H
#define GRUNTZ_COMOUTREF_H

template<class T> union ComOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

template<class T> inline void** PtrOut(T** p) {
    ComOutRef<T> r;
    r.m_asTyped = p;
    return r.m_asVoid;
}

#endif
