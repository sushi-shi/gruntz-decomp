#ifndef GRUNTZ_COMOUTREF_H
#define GRUNTZ_COMOUTREF_H

// ONE seam: an out-parameter the SDK declares `void**` fed the address of a
// TYPED pointer - IUnknown::QueryInterface's ppvObject (eleven DirectDraw /
// DirectInput sites) and CreateDIBSection's ppvBits (two). C++ will not convert
// T** to void**, so the era source wrote `(void**)&p`; this is that cast with a
// name. PtrOut is the one-expression form; the bare union is kept for the sites
// whose retail codegen shows the address materialised into a stack slot before
// the call.
template<class T> union ComOutRef {
    void** m_asVoid;
    T** m_asTyped;
};

template<class T> inline void** PtrOut(T** p) {
    ComOutRef<T> r;
    r.m_asTyped = p;
    return r.m_asVoid;
}

#endif // GRUNTZ_COMOUTREF_H
