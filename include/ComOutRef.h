#ifndef GRUNTZ_COMOUTREF_H
#define GRUNTZ_COMOUTREF_H

// A COM out-parameter's destination slot, seen both ways.
//
// IUnknown::QueryInterface fills its out-parameter through a `void**` by contract,
// while the destination is the concrete interface pointer the caller declared - two
// readings of the ONE address, and retail passes that destination's own address
// (`lea eax,[esi+N]`), never a temporary. Naming both readings keeps that shape
// without a pun at each Query.
template<class T> union ComOutRef {
    void** m_asVoid; // what QueryInterface's `void**` parameter binds to
    T** m_asTyped;   // the caller's own interface-pointer member/local
};

#endif // GRUNTZ_COMOUTREF_H
