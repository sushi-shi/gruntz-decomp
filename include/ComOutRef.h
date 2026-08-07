#ifndef GRUNTZ_COMOUTREF_H
#define GRUNTZ_COMOUTREF_H

template<class T> union ComOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

// Feed the address of a TYPED pointer to an out-parameter the SDK declares `void**`
// (CreateDIBSection's ppvBits, QueryInterface's ppvObject) without a cast at the site.
template<class T> inline void** PtrOut(T** p) {
    ComOutRef<T> r;
    r.m_asTyped = p;
    return r.m_asVoid;
}

#endif // GRUNTZ_COMOUTREF_H
